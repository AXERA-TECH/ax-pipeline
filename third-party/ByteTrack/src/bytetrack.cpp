#include "bytetrack.h"
#include "BYTETracker.h"
#include "map"
#include "memory"

struct bytetrack_handle_t
{
    std::map<int, std::shared_ptr<BYTETracker>> m_label_tracker;
    int frame_rate;
    int track_buffer;
};

bytetracker_t bytetracker_create(int frame_rate, int track_buffer)
{
    bytetrack_handle_t *handle = new bytetrack_handle_t;
    handle->frame_rate = frame_rate;
    handle->track_buffer = track_buffer;
    return handle;
}
void bytetracker_release(bytetracker_t *ptracker)
{
    if (ptracker)
    {
        bytetrack_handle_t *tracker = (bytetrack_handle_t *)*ptracker;
        delete tracker;
        tracker = nullptr;
        *ptracker = nullptr;
    }
}
void _bytetracker_track(BYTETracker *tracker, std::vector<track_object_t> &objs, std::vector<track_object_t> &track_objects, int label)
{
    auto track_results = tracker->update(objs.data(), objs.size());
    // objs->n_track_objects = track_results.size() > TRACK_OBJETCS_MAX_SIZE ? TRACK_OBJETCS_MAX_SIZE : track_results.size();

    for (size_t i = 0; i < track_results.size(); i++)
    {
        track_object_t obj = {0};
        obj.rect.x = track_results[i].tlwh[0];
        obj.rect.y = track_results[i].tlwh[1];
        obj.rect.width = track_results[i].tlwh[2];
        obj.rect.height = track_results[i].tlwh[3];
        obj.prob = track_results[i].score;
        obj.track_id = track_results[i].track_id;
        obj.user_data = track_results[i].user_data;
        obj.label = label;
        track_objects.push_back(obj);
    }
}

void bytetracker_track(bytetracker_t _tracker, bytetrack_object_t *objs)
{
    bytetrack_handle_t *tracker = (bytetrack_handle_t *)_tracker;

    std::map<int, std::vector<track_object_t>> m_label_objs;
    std::vector<track_object_t> m_track_objs;
    m_track_objs.reserve(TRACK_OBJETCS_MAX_SIZE);

    for (int i = 0; i < objs->n_objects; i++)
    {
        m_label_objs[objs->objects[i].label].push_back(objs->objects[i]);
    }

    for (auto it : m_label_objs)
    {
        if (tracker->m_label_tracker[it.first].get() == nullptr)
        {
            tracker->m_label_tracker[it.first].reset(new BYTETracker(tracker->frame_rate, tracker->track_buffer));
        }

        _bytetracker_track(tracker->m_label_tracker[it.first].get(), it.second, m_track_objs, it.first);
    }

    objs->n_track_objects = m_track_objs.size() > TRACK_OBJETCS_MAX_SIZE ? TRACK_OBJETCS_MAX_SIZE : m_track_objs.size();
    memcpy(&objs->track_objects[0], m_track_objs.data(), objs->n_track_objects * sizeof(track_object_t));
}