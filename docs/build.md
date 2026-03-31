# 构建与部署

本项目构建脚本对齐 `ax-video-sdk` 的策略: 自动下载/缓存 MSP 或 AXCL SDK；交叉编译场景会自动准备 toolchain。

## 本机(AXCL x86_64)

```bash
./build_axcl_x86.sh
```

## 本机(AXCL aarch64，例如树莓派 64 位)

在 aarch64 主机上执行该脚本会默认走本机 `g++`（不会下载 x86_64 交叉工具链）。

```bash
./build_axcl_aarch64.sh
```

## 板端(交叉编译)

AX650:

```bash
./build_ax650.sh
```

AX630C:

```bash
./build_ax630c.sh
```

## 构建产物

每个平台脚本都会生成一个可分发包:

- `artifacts/<chip>/ax_pipeline_<chip>.tar.gz`

解压后包含:

- `bin/ax_pipeline_app`
- `bin/ax_plugin_host`(当 `npu.ax_plugin_isolation="process"` 时需要)
- `lib/libax_video_sdk.so`
- `lib/plugins/libax_plugin_*.so`(内置推理插件)
- `configs/`(示例配置)

## 上板运行(示例)

```bash
scp artifacts/ax650/ax_pipeline_ax650.tar.gz root@<board_ip>:/tmp/
ssh root@<board_ip> 'mkdir -p /tmp/axp && tar -xzf /tmp/ax_pipeline_ax650.tar.gz -C /tmp/axp --strip-components=1'

# 运行时建议显式指定动态库搜索路径，避免误用板端旧库。
ssh root@<board_ip> 'LD_LIBRARY_PATH=/tmp/axp/lib /tmp/axp/bin/ax_pipeline_app -c /tmp/axp/configs/<xxx>.json -t 0'
```
