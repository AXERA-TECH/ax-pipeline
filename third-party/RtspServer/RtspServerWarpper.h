#ifndef __RTSP_H__
#define __RTSP_H__

#if __cplusplus
extern "C"
{
#endif
    typedef enum _rtsp_buffer_e
    {
        VIDEO_FRAME_I = 0x01,
        VIDEO_FRAME_P = 0x02,
        VIDEO_FRAME_B = 0x03,
    } rtsp_buffer_e;
    typedef struct _rtsp_buffer_t
    {
        void *vbuff;
        unsigned int vlen;
        unsigned long int vts;
        rtsp_buffer_e btype;

        void *abuff;
        unsigned int alen;
        unsigned long int ats;
    } rtsp_buffer_t;

    typedef void *rtsp_server_t;
    typedef unsigned int rtsp_session_t;

    rtsp_server_t rtsp_new_server(int port);
    void rtsp_rel_server(rtsp_server_t *rtsp_server);
    rtsp_session_t rtsp_new_session(rtsp_server_t rtsp_server, char *url_suffix, int h265);
    void rtsp_rel_session(rtsp_server_t rtsp_server, rtsp_session_t rtsp_session);

    int rtsp_push(rtsp_server_t rtsp_server, rtsp_session_t rtsp_session, rtsp_buffer_t *buff);
#if __cplusplus
}
#endif
#endif