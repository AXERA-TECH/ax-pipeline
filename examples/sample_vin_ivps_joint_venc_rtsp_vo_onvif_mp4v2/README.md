
## changelog 2022-11-08

sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 base on sample_vin_ivps_joint_venc_rtsp_vo_onvif.

1. add mp4v2 && click usr-button record video(mp4)!
2. rtsp more fast for vlc!
3. support onvif api! rndis usb0 need config `route add -net 239.255.255.250 netmask 255.255.255.255 usb0`

- `cd /home/examples/vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 && ./sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 -m ./model/yolov5s_sub_nv12_11.joint -p ./config/yolov5s.json -c 0 -e 2 -v 1`

```bash
root@AXERA:/home/examples/vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2# ls -l
total 43304
drwxrwxrwx 2 root root     4096 Nov  7 18:52 config
drwxrwxrwx 2 1000 1000     4096 Nov  9 18:32 model
drwxrwxrwx 2 root root     4096 Nov  9 14:04 ovf_cfg
-rw-r--r-- 1 root root 23437485 Nov  9 19:29 record_20221109192837_504.mp4
-rw-r--r-- 1 root root 19027521 Nov  9 19:30 record_20221109192932_1896.mp4
-rwxrwxrwx 1 root root  1858296 Nov  9 19:28 sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2
root@AXERA:/home/examples/vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2#
```

![result.jpg](./result.jpg)

