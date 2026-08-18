# pcd_vlm 插件设计:触发 / 选帧 / 节流 / 落库

> 约束:**单张 VLM 卡**(给客户的方案,多卡成本太高)。VLM(InternVL3.5-1B)≈3s/张、串行、超并发会 503。
> 所以核心不是"多送",而是**把每一次珍贵的 VLM 调用,用在最该看的那个目标、那一帧上**。

## 一、整体架构(检测归检测,VLM 走异步旁路)

```
每帧 infer(检测线程,绝不阻塞):
  人车非检测 → ByteTrack → 对每个 track 更新"是否候选 + 当前最优帧"
  若某 track 满足触发条件且到点 → 把它的最优ROI+元数据塞进发送队列(满就按策略丢)
  立即返回(检测/编码时间戳不受影响)

VLM worker(独立线程,单 VLM 卡):
  从队列取(可带优先级) → 全局限速 → POST ax-llm serve → 拿文本
  → 落库(jsonl/sqlite + 抓拍图);503/超时按策略丢弃或重试
```

## 二、触发与选帧策略(分五层,层层过滤)

### A. 目标级筛选 —— 哪些目标"值得"送(检测到 ≠ 送)
- **类别白名单** `classes`:只对关心的类别触发(如只送人,或只送车)。
- **置信度门槛** `min_score`:det score 太低(可能误检/模糊)不送。
- **尺寸门槛** `min_box_h`:框太小 = 像素太少,VLM 看不清 → 浪费。默认框高 < 64px 不送。
- **贴边剔除** `reject_border`:目标框贴画面边缘 = 残缺,描述不准 → 不送。
- **ROI 限定** `roi`(可选):只在指定区域(车道/门口)出现才送,忽略边角无关目标。
- **稳定性** `min_track_age`:被 ByteTrack 连续跟到 ≥N 帧才算真目标,一闪而过(误检/快速穿过)不送。

### B. 去重 —— 同一目标不要反复送(靠 track_id)
- **`per_track_once`**:每个 track_id 只送一次(首次满足条件时)。**这是省吞吐的关键**,否则一个人站那每帧都送。
- **`per_track_cooldown_s`**(可选):>0 时,长期停留的目标每 N 秒可"复看"一次(比如有人徘徊很久,值得再描述一次)。
- track 消失后清理其记录(复用之前 Kalman 那套 ~10s 过期清理,防内存膨胀)。

### C. 选帧 —— 送这个目标的哪一帧(质量优先)
决定某 track 要送后,不是抓"此刻"这帧,而是在一个短观察窗口里挑最好的:
- **`select_frame: best`**:窗口(`select_window_s`,默认 1.5s)内持续记录该 track 的"最优帧",评分 = **框面积 × score**(近而清晰的一帧),窗口结束/该发时用它。
  - 每个候选 track 只缓存**一张**最优 ROI,发送后即释放 → 内存可控。
- **`select_frame: now`**:简单模式,满足条件即用当前帧(省内存,质量略差)。
- (进阶可选:加 ROI 拉普拉斯方差挑最锐的一帧,成本略高,先不默认开。)

### D. 节流 —— 控制发送速率(单卡必须限流)
- **路级间隔** `per_stream_interval_s`:每路最小发送间隔(**拉长间隔**的主旋钮)。
- **全局速率** `global_max_rps`:所有路加起来每秒最多几次,**≈ 1/T ≈ 0.3**(VLM 吞吐上限)。多路共享一张卡,必须有全局闸。
- 二者取更严的那个生效。

### E. 丢弃 —— 队列满 / VLM 拒绝时怎么办
- **`queue_size`**:待发队列容量(小,如 4)。
- **`drop_policy`**:满时丢 `oldest`(默认,保最新更有价值)或 `newest`。
- **`on_503`**:VLM 返回 503/超时 → `drop`(记一条 miss 计数)或 `retry_once`。
- (进阶:队列改**优先级**排序,按 框面积×score 让最有价值的目标优先被 VLM 看 —— 有限吞吐下更划算。)

## 三、配置结构(plugin init_json,system prompt / VLM 参数 / 策略全可配)

```jsonc
{
  // 检测(复用现有人车非)
  "model_path": "/root/pcd.axmodel", "num_classes": 3, "strides": [16,32],
  "conf_threshold": 0.3, "enable_tracking": true,   // 去重/选帧需要 track_id

  "vlm": {
    "enable": true,
    "url": "http://127.0.0.1:8001",
    "model": "",                 // 空 = 从 /v1/models 自动取(serve 要求精确匹配)
    "system_prompt": "you are a helpful assistant.",
    "prompt": "请用不超过30个字的中文,描述图中的人、车等主要目标。",
    "max_tokens": 48, "temperature": 0.7, "top_k": 10, "repetition_penalty": 1.2,
    "timeout_ms": 8000,

    "trigger": {
      "classes": [0], "min_score": 0.4, "min_box_h": 64, "min_track_age": 8,
      "reject_border": true, "roi": null,
      "per_track_once": true, "per_track_cooldown_s": 0,
      "select_frame": "best", "select_window_s": 1.5
    },
    "rate": {
      "per_stream_interval_s": 24, "global_max_rps": 0.3,
      "queue_size": 4, "drop_policy": "oldest", "on_503": "drop"
    }
  },

  "sink": {
    "type": "jsonl", "path": "/root/vlm_records.jsonl",
    "save_crop": true, "crop_dir": "/root/vlm_crops"
  }
}
```

## 四、落库记录(每条)
`时间戳 / 路号(stream) / track_id / 类别 / 检测框 / score / 抓拍图路径 / VLM文本 / 延迟`,先 jsonl,要查询再上 sqlite。

## 五、推荐默认(单卡 InternVL3.5-1B,目标 8 路)
- `per_stream_interval_s = 24`(8 路 × 3s = 24s,刚好不堆积),`global_max_rps = 0.3`。
- `per_track_once = true` + `select_frame = best`:每个新目标,挑它最清晰的一帧,只描述一次。
- 队列 4 + drop oldest;503 直接丢并计数。
- 这样一张 VLM 卡稳稳服务 8 路,输出"每个新出现的人/车,一句 30 字中文描述 + 抓拍图"。

## 六、待确认 / 可后续加
- 触发类别到底送人、送车、还是都送?(定 `classes`)
- 是否要**行为触发**(越线/停留超时/进入 ROI 才送)——比"新目标就送"更省、更有业务意义,可作第二版。
- 落库先 jsonl 够不够,要不要简单 web 查询页。
