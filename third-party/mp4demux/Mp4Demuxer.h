#ifndef __MP4_DEMUXER_H__
#define __MP4_DEMUXER_H__

#if __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        ft_video,
        ft_audio, // unsupport yet
    } frame_type_e;
    typedef int (*mp4_frame_callback)(const void *buff, int len, frame_type_e type, void *reserve);

    typedef void *mp4_handle_t;

    mp4_handle_t mp4_open(const char *path, mp4_frame_callback cb, int loopPlay, void *reserve);

    void mp4_close(mp4_handle_t *mp4handle);

#if __cplusplus
}
#endif
#endif