#include "bytetrack.h"
#include "BYTETracker.h"

bytetracker_t bytetracker_create(int frame_rate, int track_buffer)
{
    return new BYTETracker(frame_rate, track_buffer);
}
void bytetracker_release(bytetracker_t *ptracker)
{
    if (ptracker)
    {
        BYTETracker *tracker = (BYTETracker *)*ptracker;
        delete tracker;
        tracker = nullptr;
        *ptracker = nullptr;
    }
}
void bytetracker_track(bytetracker_t _tracker, bytetrack_object_t *objs)
{
    BYTETracker *tracker = (BYTETracker *)_tracker;
    auto track_results = tracker->update(objs->objects, objs->n_objects);
    objs->n_track_objects = track_results.size() > TRACK_OBJETCS_MAX_SIZE ? TRACK_OBJETCS_MAX_SIZE : track_results.size();

    for (int i = 0; i < objs->n_track_objects; i++)
    {
        objs->track_objects[i].rect.x = track_results[i].tlwh[0];
        objs->track_objects[i].rect.y = track_results[i].tlwh[1];
        objs->track_objects[i].rect.width = track_results[i].tlwh[2];
        objs->track_objects[i].rect.height = track_results[i].tlwh[3];

        objs->track_objects[i].label = track_results[i].label;
        objs->track_objects[i].prob = track_results[i].score;
        objs->track_objects[i].track_id = track_results[i].track_id;
        objs->track_objects[i].obj_id = track_results[i].obj_id;
        objs->track_objects[i].user_data = track_results[i].user_data;
    }
}