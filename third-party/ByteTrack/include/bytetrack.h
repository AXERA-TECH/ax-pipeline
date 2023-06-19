#ifndef _H_BYTETRACKER_H_
#define _H_BYTETRACKER_H_

#ifdef __cplusplus
extern "C"
{
#endif
#define TRACK_OBJETCS_MAX_SIZE 64
    typedef struct
    {
        float x, y, width, height;
    } track_rect_t;
    typedef struct
    {
        float prob;
        track_rect_t rect;
        long track_id; // automatically generated
        void *user_data;
    } track_object_t;

    typedef struct
    {
        track_object_t objects[TRACK_OBJETCS_MAX_SIZE];
        int n_objects;

        track_object_t track_objects[TRACK_OBJETCS_MAX_SIZE];
        int n_track_objects;
    } bytetrack_object_t;

    typedef void *bytetracker_t;

    bytetracker_t bytetracker_create(int frame_rate, int track_buffer);
    void bytetracker_release(bytetracker_t *ptracker);
    void bytetracker_track(bytetracker_t tracker, bytetrack_object_t *objs);

#ifdef __cplusplus
}
#endif
#endif