 

#ifndef ONVIF_API_H
#define ONVIF_API_H


#ifdef __cplusplus
extern "C" {
#endif

void onvif_start();
void onvif_stop();
void onvif_InitPreset(void);
void onvif_AddPreset(int dev_no32, int para);
void onvif_ClePreset(int dev_no32, int para);
#ifdef __cplusplus
}
#endif

#endif


