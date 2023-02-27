# 如何加速子模块的下载？
因为子模块比较大，观察到社群有些同学，在没有魔法的时候，下载比较困难，特出此教程，希望能帮助到大家（某些用户可能会减速）。

### 获取免费的加速方式
1、打开 https://ghproxy.com/

2、浏览网页可知，通过在 git 链接前面添加 ```https://ghproxy.com/```，即可进行下载加速，

例如
```bash
git clone https://github.com/sipeed/axpi_bsp_sdk.git
```
修改成
```bash
git clone https://ghproxy.com/https://github.com/sipeed/axpi_bsp_sdk.git
```

### 修改 ```.gitmodules```
1、通过文本编辑器，打开本仓库中 ```.gitmodules``` 文件
```
[submodule "axpi_bsp_sdk"]
	path = axpi_bsp_sdk
	url = https://github.com/sipeed/axpi_bsp_sdk.git
[submodule "third-party/libv4l2cpp"]
	path = third-party/libv4l2cpp
	url = https://github.com/mpromonet/libv4l2cpp.git
[submodule "third-party/RTSP"]
	path = third-party/RTSP
	url = https://github.com/ZHEQIUSHUI/RTSP.git
```
2、通过上述方式，将 ```https://ghproxy.com/``` 添加到 url 字段后面对应的 git 链接上，结果如下
```
[submodule "axpi_bsp_sdk"]
	path = axpi_bsp_sdk
	url = https://ghproxy.com/https://github.com/sipeed/axpi_bsp_sdk.git
[submodule "third-party/libv4l2cpp"]
	path = third-party/libv4l2cpp
	url = https://ghproxy.com/https://github.com/mpromonet/libv4l2cpp.git
[submodule "third-party/RTSP"]
	path = third-party/RTSP
	url = https://ghproxy.com/https://github.com/ZHEQIUSHUI/RTSP.git
```
