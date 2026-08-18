#!/usr/bin/env python3
"""
演示 feeder —— 模拟检测插件,不需要真机就能看端到端效果。
从一个帧目录里取图,当作"抓拍",附上随机的类别/框/置信度,POST 到 web 的 /ingest。
不带 desc,所以由 web 侧补调 VLM(演示真实描述)。

用法:
    python feed_demo.py --ingest http://127.0.0.1:8900/ingest --frames_dir /tmp \
        --pattern 'vf*.jpg' --streams 4 --interval 8 --mode single
"""
import os, glob, time, base64, json, random, argparse, urllib.request

def b64(p):
    return base64.b64encode(open(p, "rb").read()).decode()

def post(url, item):
    data = json.dumps(item).encode()
    req = urllib.request.Request(url, data=data, headers={"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            return r.status
    except Exception as e:
        return f"ERR {e}"

def rand_box():
    x0 = random.randint(30, 1400); y0 = random.randint(60, 700)
    return [x0, y0, x0 + random.randint(80, 300), y0 + random.randint(90, 320)]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ingest", default="http://127.0.0.1:8900/ingest")
    ap.add_argument("--frames_dir", default="/tmp")
    ap.add_argument("--pattern", default="vf*.jpg")
    ap.add_argument("--streams", type=int, default=4)
    ap.add_argument("--interval", type=float, default=8.0, help="每路两次抓拍的间隔(秒)")
    ap.add_argument("--mode", choices=["single", "clip"], default="single")
    ap.add_argument("--clip_frames", type=int, default=3)
    a = ap.parse_args()

    frames = sorted(glob.glob(os.path.join(a.frames_dir, a.pattern)))
    if not frames:
        raise SystemExit(f"no frames matched {a.frames_dir}/{a.pattern}")
    streams = [f"CH{i+1:02d}" for i in range(a.streams)]
    print(f"feeder -> {a.ingest} | {len(frames)} 帧 | {a.streams} 路 | 每路 {a.interval}s | {a.mode}")
    tid = 1000
    gap = a.interval / max(1, a.streams)
    while True:
        for ch in streams:
            item = {"stream": ch, "cls": random.choice([0, 1, 1, 2]),
                    "score": round(random.uniform(0.5, 0.96), 2),
                    "box": rand_box(), "track_id": tid, "ts": time.time()}
            if a.mode == "clip":
                item["frames"] = [b64(random.choice(frames)) for _ in range(a.clip_frames)]
            else:
                item["image"] = b64(random.choice(frames))
            st = post(a.ingest, item)
            print(f"  {ch} track{tid} cls{item['cls']} -> {st}")
            tid += 1
            time.sleep(gap)

if __name__ == "__main__":
    main()
