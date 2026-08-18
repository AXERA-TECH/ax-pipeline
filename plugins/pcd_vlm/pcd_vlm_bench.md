# pcd_vlm 插件 —— VLM 选型与调参实测

> 目的:给「检测触发 → VLM 解析 → 记录」这条链路选一个又快又准的小 VLM,并定下输出长度/防复读/并发的参数。
> 所有数据在 8 卡 AX650(AXCL)x86 主机上实测,axllm commit `29427ac`(build_x86)。

## 测试方法

- 服务:`AXLLM_DEVICES=<card> axllm serve <model_dir> --port <port>`,OpenAI 兼容接口 `POST /v1/chat/completions`。
- 每卡一个模型并行测;图片走 `image_url` 的 `data:image/jpeg;base64,...`。
- 测试图:一张 1080p 泰国路口图(人 / 车 / 摩托 / 出租 / 厢式车,典型人车非场景)。
- 统一 prompt(下称 **B**):`请用不超过30个字的中文,描述图中的人、车等主要目标。`
- `max_tokens=56`;采样用各模型 `post_config.json` 默认(InternVL3.5-1B:`temperature=0.7 / top_k=10 / repetition_penalty=1.2 / penalty_window=30`,top_p 关)。
- `t_s` = 单张端到端墙钟(含 vision 编码 + prefill + decode);`tok/s` = completion_tokens / t_s(端到端等效,非纯 decode)。

## 横评结果(同图同 prompt)

| 模型 | 卡 | t_s | completion / prompt tokens | 输出字数 | 端效 tok/s | 结论 |
|---|---|---:|---|---:|---:|---|
| **InternVL3.5-1B (AX650)** | 0 | **3.06** | 20 / 299 | 31 | 6.5 | ✅ **首选**:最快 + 最准 + 中文短描述达标 |
| Qwen3-VL-2B-Instruct | 2 | 3.28 | 17 / 185 | 25 | 5.2 | ✅ 次选:流畅;vision token 少(185)所以 2B 也不慢 |
| InternVL3.5-2B | 1 | 4.54 | 17 / 299 | 28 | 3.7 | ⚠️ 比 1B 慢一半,质量没更好,不划算 |
| FastVLM-1.5B | 3 | 3.93 | 34 / 297 | 60 | 8.6 | ⚠️ 正确但压不住长度(会带出高楼/电线杆/树) |
| gemma-4-E2B | 4 | 10.0 | 56 / 112 | — | 5.6 | ❌ 乱码 `وفر져` + 慢,此用法不可用(疑 chat 模板不匹配) |
| SmolVLM2-500M | 5 | 5.18 | 56 / 378 | 32 | 10.8 | ❌ 太弱,啰嗦且截断 |
| InternVL3.5-1B **AX620E 版** | 7 | — | — | — | — | ❌ 加载失败 `AXCLWorker exit`(AX620E 模型不能跑在 AX650/AXCL) |

**样例输出(InternVL3.5-1B):** `图中主要目标包括:骑摩托车的人、黄色出租车、白色厢式车和行人。`

## 关键发现

1. **首选 InternVL3.5-1B(AX650 版)** —— 速度、正确性、中文短描述三项都最好。
2. **输出长度可控**:prompt B + `max_tokens 48~56` → 稳定 25~31 字、一句话、完整不截断。要更短就把"30字"改小并把 max_tokens 降到 ~40。
3. **防复读**:`repetition_penalty=1.2`(post_config 默认)已足够;`max_tokens=200` 也只说 30 字就自然收尾,不会啰嗦凑数。FastVLM/SmolVLM 的"长"是模型本身话多,不是复读。
4. **并发 = 串行**:serve 启动日志 `Max concurrency: 1`。实测同时打 4 个 → 2 个串行成功(3.07s→6.15s,内容一致正确)+ 2 个直接 `503 Service Unavailable`。
   - 即 **NPU 不会并行推理、不会因并发胡说**;但超并发的请求是被拒(503),不是无限排队 → 客户端要处理 503(丢弃/重试)。
5. **速度瓶颈在 vision 编码 + prefill**(~2.3s 固定),decode 只占小头 → 压缩输出 token 收益有限。想更快只能:换 vision token 更少的模型(如 Qwen3-VL-2B)、降输入分辨率、或减少图 tile。

## 负载换算(决定能跑几路)

单卡 VLM 串行,不堆积条件:**触发间隔 ≥ T × 路数**(秒)。取 T≈3s(InternVL3.5-1B):

| 路数 | 10s 间隔 | 需要的最小间隔 |
|---:|---|---|
| 3 路 | 刚好(3×3=9s<10s)✅ | ~9s |
| 8 路 | 会堆积(需24s)❌ | ~24s |
| 16 路 | 会堆积 ❌ | ~48s |

**结论**:10s/路 + 单卡 VLM 只够 ~3 路。要 8~16 路,要么把触发间隔拉长(24~48s),要么多张卡分担 VLM,要么接受"抽样触发"(队列满就丢)。这跟"检测归检测卡、VLM 归 VLM 卡"的双卡思路一致:VLM 就是瓶颈,按它的吞吐喂。

## 给 pcd_vlm 插件的推荐配置

- 模型:`InternVL3_5-1B_GPTQ_INT4-AX650`(备选 `Qwen3-VL-2B-Instruct`)。
- 服务:独占一张卡 `AXLLM_DEVICES=<card> axllm serve <dir> --port 8001`。
- 请求:prompt = "请用不超过30个字的中文,描述图中的人、车等主要目标。",`max_tokens=48`,model 名从 `/v1/models` 动态取(serve 要求精确匹配)。
- 采样:用 post_config 默认(temp 0.7 / top_k 10 / rep_penalty 1.2),防复读够用。
- 触发:每路节流间隔**默认 ≥ (路数 × 3)s**;插件内异步单槽队列,满即丢;对 503 做丢弃或一次重试。

## 附:多帧视频理解(可选模式,Qwen3-VL-2B)

支持视频/多帧且有 token 预算规划的:**Qwen3-VL-2B**(`QwenVLAdapter` + `VideoPlanKind::SimpleBudgetFit`,多帧共享预算、每帧自动降分辨率)、Qwen2.5VL、Gemma4VL、MiniCPM-V-4.6。InternVL3.5 / SmolVLM / FastVLM 无 `videoPlanKind` —— 实测 **InternVL3.5-1B 发 `video:` 多帧直接报错(HTTP 5xx),只能单帧**。
请求:每帧一个 `{"type":"image_url","image_url":{"url":"video:data:image/jpeg;base64,..."}}`;帧数上限 `num_frames` / config `vision_num_frames`。

实测(同一路口,Qwen3-VL-2B,card7:8013):

| 模式 | prompt_tokens | t_s | 输出 |
|---|---:|---:|---|
| 单帧 | 179 | 2.67 | 城市街道上,黄色出租车和各种车辆在众多电线中穿行。 |
| 视频 3 帧 | 334 (1.86×) | 3.69 | 各种车辆和行人正在繁忙地穿行。 |
| 视频 5 帧 | 478 (2.67×) | 4.28 | 各种车辆和行人正在繁忙地穿行。 |
| 视频 5 帧(间隔10s) | 496 | 4.36 | 车辆与行人交替通过路口,车流在路口间来回穿梭。 |

**关键结论:**
1. **多帧 token 次线性**:5 帧只 ≈ 2.67× 单帧(不是 5×)—— 印证"多帧用小分辨率编码,总 token 不爆"。
2. **耗时只多一点**:5 帧 4.3s vs 单帧 2.7s。→ "30s 一次、一次丢 3~5 帧"完全可行,耗时和单帧一个量级。
3. **但动态描述有限**:2B 模型对繁忙交通只能给概括性时序("来回穿梭""交替通过"),做不到精确目标级轨迹("红车从左驶入、行人穿过斑马线")。比单帧略有时序感,提升不大。要精确动态需更大视频模型 —— 与"单卡低成本"矛盾。

**建议**:插件把取帧模式做成可配 `frame_mode: single(best 选帧) | clip(多帧视频)`。概括记录"这段时间大致发生了什么"用 `clip` 够用且更稳(不怕单帧糊/遮挡);要精确动作识别则 2B 力有不逮。
