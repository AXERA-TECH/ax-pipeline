# RTSP 预览与排障

## URL 格式

在配置里写:

```json
{ "uris": ["rtsp://0.0.0.0:8554/demo"] }
```

表示在本机/板子上监听 `8554` 端口，并对外提供路径 `/demo`。

## 预览

推荐用 TCP 拉流:

```bash
ffplay -rtsp_transport tcp rtsp://<ip>:8554/demo
```

或用:

```bash
ffprobe -rtsp_transport tcp rtsp://<ip>:8554/demo
```

## 常见问题

- `Connection refused`
  - 检查 `8554` 是否被其它进程占用: `ss -ltnp | grep 8554`
  - 某些环境下 docker 会抢占端口
- 远端机器打不开但本机能打开
  - 检查防火墙/路由
  - 确认绑定为 `0.0.0.0` 而不是 `127.0.0.1`

