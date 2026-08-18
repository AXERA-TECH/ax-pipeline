#!/usr/bin/env python3
"""
pcd_vlm 事件中心 (方案B 的"大脑")
--------------------------------------------------
职责:接收检测插件推来的候选目标(ROI 抓拍图 + 元数据) -> 全局限流队列 ->
调用 ax-llm VLM serve 生成一句中文描述 -> 落库(SQLite)+ 存图 -> SSE 实时推送 + 网页展示。

插件(C++)保持无状态:只做检测/选帧,把候选 POST 到 /ingest。
全局 VLM 调度、限流、去重、落库、展示都集中在这里(单进程 = 天然全局)。

启动:
    pip install -r requirements.txt
    VLM_URL=http://127.0.0.1:8013 python server.py            # 默认监听 0.0.0.0:8900
配置全部走环境变量,见下方 CFG。
"""
import os, json, sqlite3, asyncio, time, base64, contextlib
from pathlib import Path
from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse, StreamingResponse, JSONResponse, Response
import httpx, uvicorn

HERE = Path(__file__).resolve().parent

CFG = {
    "vlm_url":       os.environ.get("VLM_URL", "http://127.0.0.1:8013"),  # OpenAI 兼容端点: ax-llm / vLLM / 云端付费API
    "vlm_api_key":   os.environ.get("VLM_API_KEY", ""),               # 云端/vLLM 需要时填,ax-llm 本地留空
    "vlm_model":     os.environ.get("VLM_MODEL", ""),                 # 空 = 从 /v1/models 自动取; 云端填固定名如 gpt-4o
    "clip_style":    os.environ.get("CLIP_STYLE", "axllm"),           # 多帧格式: axllm(video:前缀) | openai(普通多图)
    "frame_mode":    os.environ.get("FRAME_MODE", "single"),         # single | clip
    "system_prompt": os.environ.get("SYS_PROMPT", "you are a helpful assistant."),
    "prompt":        os.environ.get("PROMPT", "请用不超过30个字的中文,描述图中的人、车等主要目标。"),
    "prompt_clip":   os.environ.get("PROMPT_CLIP", "下面是同一路口每隔约10秒的连续几帧。请用中文一句话说明这段时间里主要的人和车怎么移动或变化(如驶入/离开/穿过路口),30字内。"),
    "max_tokens":    int(os.environ.get("MAX_TOKENS", "48")),
    "temperature":   float(os.environ.get("TEMPERATURE", "0.7")),
    "global_max_rps":float(os.environ.get("MAX_RPS", "0.5")),        # 全局 VLM 速率上限(单卡吞吐)
    "queue_size":    int(os.environ.get("QUEUE_SIZE", "8")),
    "dedup_sec":     float(os.environ.get("DEDUP_SEC", "0")),        # 同一(路,track)N秒内只处理一次;0=关
    "db":            os.environ.get("DB", str(HERE / "events.db")),
    "imgdir":        os.environ.get("IMGDIR", str(HERE / "snapshots")),
    "port":          int(os.environ.get("PORT", "8900")),
}
CLS_CN = {0: "人", 1: "车", 2: "非机动车"}

Path(CFG["imgdir"]).mkdir(parents=True, exist_ok=True)

# ---------------- storage ----------------
def db():
    c = sqlite3.connect(CFG["db"])
    c.row_factory = sqlite3.Row
    return c

MAX_EVENTS = int(os.environ.get("MAX_EVENTS", "100"))   # 只保留最近 N 个事件(DB+抓拍文件同步清理)

def init_db():
    with db() as c:
        c.execute("""CREATE TABLE IF NOT EXISTS events(
            id INTEGER PRIMARY KEY AUTOINCREMENT, ts REAL, stream TEXT, track_id INTEGER,
            cls INTEGER, score REAL, box TEXT, latency_ms INTEGER, desc TEXT, img TEXT, mode TEXT,
            replay TEXT)""")
        try:
            c.execute("ALTER TABLE events ADD COLUMN replay TEXT")   # 旧库平滑升级
        except sqlite3.OperationalError:
            pass
        c.execute("CREATE INDEX IF NOT EXISTS idx_ts ON events(ts DESC)")

def prune_events(c):
    """只保留最近 MAX_EVENTS 条,连同抓拍/轮播图片文件一起删,防磁盘无限膨胀。"""
    rows = c.execute("SELECT id,img,replay FROM events ORDER BY id DESC LIMIT -1 OFFSET ?",
                     (MAX_EVENTS,)).fetchall()
    for r in rows:
        for name in [r["img"]] + json.loads(r["replay"] or "[]"):
            if name:
                with contextlib.suppress(OSError):
                    os.remove(os.path.join(CFG["imgdir"], name))
        c.execute("DELETE FROM events WHERE id=?", (r["id"],))

# ---------------- global state ----------------
Q: "asyncio.Queue" = None
SUBS: set = set()          # SSE subscriber queues
STATS = {"received": 0, "processed": 0, "dropped": 0, "errors": 0, "queue": 0}
LAST_SEEN = {}             # (stream,track)->ts  for dedup
MODEL = None

def auth_headers():
    return {"Authorization": "Bearer " + CFG["vlm_api_key"]} if CFG["vlm_api_key"] else {}

async def resolve_model(client):
    global MODEL
    if CFG["vlm_model"]:
        MODEL = CFG["vlm_model"]; return
    try:
        r = await client.get(CFG["vlm_url"].rstrip("/") + "/v1/models", headers=auth_headers(), timeout=10)
        MODEL = r.json()["data"][0]["id"]
    except Exception as e:
        MODEL = "vlm"; print("[warn] resolve model failed:", e)

def build_body(item):
    """item: {image | frames}. single -> one image_url; clip -> multi-frame (video:前缀 or 普通多图)."""
    if item.get("frames"):
        pre = "video:data:image/jpeg;base64," if CFG["clip_style"] == "axllm" else "data:image/jpeg;base64,"
        parts = [{"type": "image_url", "image_url": {"url": pre + f}} for f in item["frames"]]
        prompt = CFG["prompt_clip"]
    else:
        parts = [{"type": "image_url", "image_url": {"url": "data:image/jpeg;base64," + item["image"]}}]
        prompt = CFG["prompt"]
    parts.append({"type": "text", "text": prompt})
    return {"model": MODEL, "max_tokens": CFG["max_tokens"], "temperature": CFG["temperature"],
            "stream": False,
            "messages": [{"role": "system", "content": [{"type": "text", "text": CFG["system_prompt"]}]},
                         {"role": "user", "content": parts}]}

async def call_vlm(client, item):
    t0 = time.time()
    r = await client.post(CFG["vlm_url"].rstrip("/") + "/v1/chat/completions",
                          json=build_body(item), headers=auth_headers(), timeout=120)
    r.raise_for_status()
    c = r.json()["choices"][0]["message"]["content"]
    if isinstance(c, list):
        c = "".join(x.get("text", "") for x in c)
    return (c or "").strip(), int((time.time() - t0) * 1000)

async def broadcast(ev):
    for q in list(SUBS):
        with contextlib.suppress(Exception):
            q.put_nowait(ev)

async def persist_and_broadcast(item, desc, ms, mode):
    """写图 + 落库 + SSE 推送。插件已带 desc(方案A) 或 web 补调后(方案B) 都走这里。"""
    stamp = f"{int(item['ts']*1000)}_{item.get('stream','')}_{item.get('track_id',0)}"
    img_b64 = item.get("image") or (item.get("frames") or [None])[0]
    img_name = ""
    if img_b64:
        img_name = f"{stamp}.jpg"
        with open(os.path.join(CFG["imgdir"], img_name), "wb") as f:
            f.write(base64.b64decode(img_b64))
    # 轮播帧(插件送来的 ±2s 每秒1帧)逐张落盘
    replay_names = []
    for i, fb64 in enumerate(item.get("replay") or []):
        name = f"{stamp}_r{i}.jpg"
        with open(os.path.join(CFG["imgdir"], name), "wb") as f:
            f.write(base64.b64decode(fb64))
        replay_names.append(name)
    with db() as c:
        cur = c.execute("INSERT INTO events(ts,stream,track_id,cls,score,box,latency_ms,desc,img,mode,replay)"
                        " VALUES(?,?,?,?,?,?,?,?,?,?,?)",
                        (item["ts"], item.get("stream", ""), item.get("track_id", -1),
                         item.get("cls", -1), item.get("score", 0), json.dumps(item.get("box", [])),
                         ms, desc, img_name, mode, json.dumps(replay_names)))
        eid = cur.lastrowid
        prune_events(c)          # 滚动清理:只留最近 MAX_EVENTS 条(含图片文件)
    STATS["processed"] += 1
    await broadcast({"id": eid, "ts": item["ts"], "stream": item.get("stream", ""),
                     "track_id": item.get("track_id", -1), "cls": item.get("cls", -1),
                     "score": item.get("score", 0), "box": item.get("box", []),
                     "latency_ms": ms, "desc": desc, "img": img_name, "mode": mode,
                     "replay": replay_names})
    return eid

async def worker():
    interval = 1.0 / CFG["global_max_rps"] if CFG["global_max_rps"] > 0 else 0
    last = 0.0
    async with httpx.AsyncClient() as client:
        await resolve_model(client)
        print(f"[worker] VLM={CFG['vlm_url']} model={MODEL} mode={CFG['frame_mode']} max_rps={CFG['global_max_rps']}")
        while True:
            item = await Q.get()
            STATS["queue"] = Q.qsize()
            wait = interval - (time.time() - last)
            if wait > 0:
                await asyncio.sleep(wait)
            last = time.time()
            try:
                desc, ms = await call_vlm(client, item)
            except Exception as e:
                STATS["errors"] += 1
                print("[vlm err]", e); continue
            await persist_and_broadcast(item, desc, ms, "clip" if item.get("frames") else "single")

# ---------------- app ----------------
app = FastAPI()

@app.on_event("startup")
async def _startup():
    global Q
    Q = asyncio.Queue(maxsize=CFG["queue_size"])
    init_db()
    asyncio.create_task(worker())

@app.post("/ingest")
async def ingest(req: Request):
    """插件推候选目标。body: {stream,track_id,cls,score,box,ts,image(b64) | frames:[b64,...]}"""
    item = await req.json()
    item.setdefault("ts", time.time())
    if not (1e9 < float(item["ts"]) < 5e9):   # 非 epoch(如单调钟)一律用服务器时间兜底
        item["ts"] = time.time()
    STATS["received"] += 1
    if CFG["dedup_sec"] > 0:
        k = (item.get("stream"), item.get("track_id"))
        now = item["ts"]
        if k in LAST_SEEN and now - LAST_SEEN[k] < CFG["dedup_sec"]:
            return JSONResponse({"status": "deduped"}, status_code=200)
        LAST_SEEN[k] = now
    # 方案A: 插件已带描述 -> 直接展示,不再排队调 VLM
    if item.get("desc"):
        await persist_and_broadcast(item, item["desc"], item.get("latency_ms", 0), item.get("mode", "single"))
        return JSONResponse({"status": "stored"}, status_code=202)
    # 方案B / 演示: 只有图 -> 入队,由 web 补调 VLM
    try:
        Q.put_nowait(item)
    except asyncio.QueueFull:
        STATS["dropped"] += 1
        return JSONResponse({"status": "dropped", "reason": "queue_full"}, status_code=429)
    return JSONResponse({"status": "queued", "queue": Q.qsize()}, status_code=202)

@app.get("/api/stats")
async def stats():
    STATS["queue"] = Q.qsize() if Q else 0
    return {**STATS, "model": MODEL, "cfg": {k: CFG[k] for k in
            ("frame_mode", "global_max_rps", "queue_size", "max_tokens", "prompt")}}

@app.get("/api/events")
async def events(limit: int = 100, stream: str = "", cls: int = -1):
    q = "SELECT * FROM events"
    conds, args = [], []
    if stream:
        conds.append("stream=?"); args.append(stream)
    if cls >= 0:
        conds.append("cls=?"); args.append(cls)
    if conds:
        q += " WHERE " + " AND ".join(conds)
    q += " ORDER BY id DESC LIMIT ?"; args.append(limit)
    with db() as c:
        rows = [dict(r) for r in c.execute(q, args)]
    for r in rows:
        r["box"] = json.loads(r["box"] or "[]")
        r["replay"] = json.loads(r.get("replay") or "[]")
    return rows

@app.get("/img/{name}")
async def img(name: str):
    p = os.path.join(CFG["imgdir"], os.path.basename(name))
    if not os.path.exists(p):
        return Response(status_code=404)
    return Response(open(p, "rb").read(), media_type="image/jpeg")

@app.get("/stream")
async def sse():
    q = asyncio.Queue(); SUBS.add(q)
    async def gen():
        try:
            yield "retry: 3000\n\n"
            while True:
                ev = await q.get()
                yield f"data: {json.dumps(ev, ensure_ascii=False)}\n\n"
        finally:
            SUBS.discard(q)
    return StreamingResponse(gen(), media_type="text/event-stream")

@app.get("/", response_class=HTMLResponse)
async def index():
    return (HERE / "static" / "index.html").read_text(encoding="utf-8")

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=CFG["port"])
