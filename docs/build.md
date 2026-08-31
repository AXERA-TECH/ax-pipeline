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

## AXCL riscv64（交叉编译）

该模式需要：

- `riscv64-unknown-linux-gnu-gcc/g++` 可在 `PATH` 中找到
- `AXSDK_AXCL_DIR` 指向 riscv 版 AXCL SDK root（包含 `include/axcl/axcl.h` 与 `lib/axcl/libaxcl_sys.so`）

```bash
export AXSDK_AXCL_DIR=/path/to/axcl_linux_riscv
./build_axcl_riscv64.sh
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

## BSP 版本检查

编译时会从所用 SDK 库中提取版本号(板端取 `libax_sys.so`,AXCL 取 `libaxcl_rt.so`)烧进二进制;启动时与运行环境版本对比(板端读 `/proc/ax_proc/version`,AXCL 调 `axclrtGetVersion`):

- **板端(MSP)**:主版本不一致直接拒绝启动(exit 4)——BSP 固件与编译库不匹配会产生花屏、驱动 hang 等不可控错误,宁可拦在第一步。
- **AXCL**:host runtime 是版本化稳定接口,跨小版本混用受支持,不一致仅打印告警。

已确认可用的临近版本组合(如 3.10.2 配 3.16)可用环境变量跳过:

```bash
AX_BSP_VERSION_CHECK_SKIP=1 ./ax_pipeline_app -c xxx.json
```

当前版本也显示在控制台网页系统栏与 `GET /api/v1/system` 的 `bsp` 字段。
