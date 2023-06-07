#include "../include/ax_model_base.hpp"
#include "../../utilities/json.hpp"

#include "utilities/object_register.hpp"
#include "../../utilities/sample_log.h"
// #include "ax_sys_api.h"
#include "ax_common_api.h"
#include "fstream"

#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

std::map<std::string, int> ModelTypeTable = {
    {"MT_UNKNOWN", MT_UNKNOWN},
};
#include "ax_model_det.hpp"
#include "ax_model_crowdcount.hpp"
#include "ax_model_seg.hpp"
#include "ax_model_multi_level_model.hpp"
#include "ax_model_ml_sub.hpp"

#include "ax_model_runner_ax620.hpp"
#include "ax_model_runner_ax650.hpp"
#include "ax_model_base.hpp"

template <typename T>
void update_val(nlohmann::json &jsondata, const char *key, T *val)
{
    if (jsondata.contains(key))
    {
        *val = jsondata[key];
    }
}

template <typename T>
void update_val(nlohmann::json &jsondata, const char *key, std::vector<T> *val)
{
    if (jsondata.contains(key))
    {
        std::vector<T> tmp = jsondata[key];
        *val = tmp;
    }
}

int ax_model_base::get_model_type(void *json_obj, std::string &strModelType)
{
    MODEL_TYPE_E m_model_type = MT_UNKNOWN;
    auto jsondata = *(nlohmann::json *)json_obj;
    if (jsondata.contains("MODEL_TYPE"))
    {
        if (jsondata["MODEL_TYPE"].is_number_integer())
        {
            int mt = -1;
            mt = jsondata["MODEL_TYPE"];
            auto it = ModelTypeTable.begin();
            for (size_t i = 0; i < ModelTypeTable.size(); i++)
            {
                if (it->second == mt)
                {
                    m_model_type = (MODEL_TYPE_E)mt;
                }
            }
            // m_model_type = (MODEL_TYPE_E)mt;
        }
        else if (jsondata["MODEL_TYPE"].is_string())
        {
            strModelType = jsondata["MODEL_TYPE"];

            auto item = ModelTypeTable.find(strModelType);

            if (item != ModelTypeTable.end())
            {
                m_model_type = (MODEL_TYPE_E)ModelTypeTable[strModelType];
            }
        }
    }
    return m_model_type;
}

int ax_model_base::get_runner_type(void *json_obj, std::string &strRunnerType)
{
    RUNNER_TYPE_E m_runner_type = RUNNER_UNKNOWN;
    auto jsondata = *(nlohmann::json *)json_obj;
    if (jsondata.contains("RUNNER_TYPE"))
    {
        if (jsondata["RUNNER_TYPE"].is_number_integer())
        {
            int mt = -1;
            mt = jsondata["RUNNER_TYPE"];
            auto it = ModelTypeTable.begin();
            for (size_t i = 0; i < ModelTypeTable.size(); i++)
            {
                if (it->second == mt)
                {
                    m_runner_type = (RUNNER_TYPE_E)mt;
                }
            }
            // m_model_type = (MODEL_TYPE_E)mt;
        }
        else if (jsondata["RUNNER_TYPE"].is_string())
        {
            strRunnerType = jsondata["RUNNER_TYPE"];

            auto item = ModelTypeTable.find(strRunnerType);

            if (item != ModelTypeTable.end())
            {
                m_runner_type = (RUNNER_TYPE_E)ModelTypeTable[strRunnerType];
            }
        }
    }
    return m_runner_type;
}

bool ax_model_base::get_track_enable(void *json_obj)
{
    auto jsondata = *(nlohmann::json *)json_obj;
    if (jsondata.contains("TRACK_ENABLE"))
    {
        bool enable = jsondata["TRACK_ENABLE"];
        return enable;
    }
    return false;
}

void ax_model_base::enable_track(int frame_rate, int track_buffer)
{
    disbale_track();
    tracker = bytetracker_create(frame_rate, track_buffer);
    b_track = true;
}

void ax_model_base::disbale_track()
{
    if (tracker)
    {
        bytetracker_release(&tracker);
        tracker = nullptr;
    }
    b_track = false;
}

void ax_model_base::draw_bbox(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    int x, y;
    cv::Size label_size;
    int baseLine = 0;
    for (int i = 0; i < results->nObjSize; i++)
    {
        cv::Rect rect(results->mObjects[i].bbox.x * image.cols + offset_x,
                      results->mObjects[i].bbox.y * image.rows + offset_y,
                      results->mObjects[i].bbox.w * image.cols,
                      results->mObjects[i].bbox.h * image.rows);
        std::string label_str = results->mObjects[i].objname;
        if (b_track)
        {
            label_str += " " + std::to_string(results->mObjects[i].track_id);
        }

        label_size = cv::getTextSize(label_str, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);
        if (results->mObjects[i].bHasBoxVertices)
        {
            cv::line(image,
                     cv::Point(results->mObjects[i].bbox_vertices[0].x * image.cols + offset_x, results->mObjects[i].bbox_vertices[0].y * image.rows + offset_y),
                     cv::Point(results->mObjects[i].bbox_vertices[1].x * image.cols + offset_x, results->mObjects[i].bbox_vertices[1].y * image.rows + offset_y),
                     cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);
            cv::line(image,
                     cv::Point(results->mObjects[i].bbox_vertices[1].x * image.cols + offset_x, results->mObjects[i].bbox_vertices[1].y * image.rows + offset_y),
                     cv::Point(results->mObjects[i].bbox_vertices[2].x * image.cols + offset_x, results->mObjects[i].bbox_vertices[2].y * image.rows + offset_y),
                     cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);
            cv::line(image,
                     cv::Point(results->mObjects[i].bbox_vertices[2].x * image.cols + offset_x, results->mObjects[i].bbox_vertices[2].y * image.rows + offset_y),
                     cv::Point(results->mObjects[i].bbox_vertices[3].x * image.cols + offset_x, results->mObjects[i].bbox_vertices[3].y * image.rows + offset_y),
                     cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);
            cv::line(image,
                     cv::Point(results->mObjects[i].bbox_vertices[3].x * image.cols + offset_x, results->mObjects[i].bbox_vertices[3].y * image.rows + offset_y),
                     cv::Point(results->mObjects[i].bbox_vertices[0].x * image.cols + offset_x, results->mObjects[i].bbox_vertices[0].y * image.rows + offset_y),
                     cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);

            x = results->mObjects[i].bbox_vertices[0].x * image.cols + offset_x;
            y = results->mObjects[i].bbox_vertices[0].y * image.rows + offset_y - label_size.height - baseLine;
        }
        else
        {
            cv::rectangle(image, rect, COCO_COLORS[results->mObjects[i].label % COCO_COLORS.size()], thickness);
            x = rect.x;
            y = rect.y - label_size.height - baseLine;
        }

        if (y < 0)
            y = 0;
        if (x + label_size.width > image.cols)
            x = image.cols - label_size.width;

        cv::rectangle(image, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                      cv::Scalar(255, 255, 255, 255), -1);

        cv::putText(image, label_str, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale,
                    cv::Scalar(0, 0, 0, 255), thickness);
    }
}

void ax_model_base::draw_fps(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    sprintf(fps_info, "fps:%02d", results->niFps);
    cv::Size label_size = cv::getTextSize(fps_info, cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5, thickness * 2, NULL);
    cv::putText(image, fps_info, cv::Point(0, label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5,
                cv::Scalar(255, 0, 255, 0), thickness * 2);
}

void ax_model_base::draw_bbox(int chn, axdl_results_t *results, float fontscale, int thickness)
{
    for (size_t d = 0; d < results->nObjSize; d++)
    {
        if (results->mObjects[d].bHasBoxVertices)
        {
            if (results->bObjTrack)
                m_drawers[chn].add_polygon(results->mObjects[d].bbox_vertices, 4, COCO_COLORS_ARGB[results->mObjects[d].track_id % COCO_COLORS_ARGB.size()], thickness);
            else
                m_drawers[chn].add_polygon(results->mObjects[d].bbox_vertices, 4, COCO_COLORS_ARGB[results->mObjects[d].label % COCO_COLORS_ARGB.size()], thickness);

            if (results->bObjTrack && b_draw_obj_name)
            {
                m_drawers[chn].add_text(std::string(results->mObjects[d].objname) + " " + std::to_string(results->mObjects[d].track_id),
                                        results->mObjects[d].bbox_vertices[0],
                                        {UCHAR_MAX, 0, 0, 0}, fontscale, 2);
            }
            else if (b_draw_obj_name)
            {
                m_drawers[chn].add_text(results->mObjects[d].objname,
                                        results->mObjects[d].bbox_vertices[0],
                                        {UCHAR_MAX, 0, 0, 0}, fontscale, 2);
            }
        }
        else
        {
            if (results->bObjTrack)
                m_drawers[chn].add_rect(&results->mObjects[d].bbox, COCO_COLORS_ARGB[results->mObjects[d].track_id % COCO_COLORS_ARGB.size()], thickness);
            else
                m_drawers[chn].add_rect(&results->mObjects[d].bbox, COCO_COLORS_ARGB[results->mObjects[d].label % COCO_COLORS_ARGB.size()], thickness);

            if (results->bObjTrack && b_draw_obj_name)
            {
                m_drawers[chn].add_text(std::string(results->mObjects[d].objname) + " " + std::to_string(results->mObjects[d].track_id),
                                        {results->mObjects[d].bbox.x, results->mObjects[d].bbox.y},
                                        {UCHAR_MAX, 0, 0, 0}, fontscale, 2);
            }
            else if (b_draw_obj_name)
            {
                m_drawers[chn].add_text(results->mObjects[d].objname,
                                        {results->mObjects[d].bbox.x, results->mObjects[d].bbox.y},
                                        {UCHAR_MAX, 0, 0, 0}, fontscale, 2);
            }
        }
    }
}

void ax_model_base::draw_fps(int chn, axdl_results_t *results, float fontscale, int thickness)
{
    m_drawers[chn].add_text("fps:" + std::to_string(results->niFps),
                            {0, 0},
                            {UCHAR_MAX, 0, 0, 0}, 1, 2);
}

int ax_model_single_base_t::init(void *json_obj)
{
    auto jsondata = *(nlohmann::json *)json_obj;

    update_val(jsondata, "PROB_THRESHOLD", &PROB_THRESHOLD);
    update_val(jsondata, "NMS_THRESHOLD", &NMS_THRESHOLD);
    update_val(jsondata, "CLASS_NUM", &CLASS_NUM);
    update_val(jsondata, "ANCHORS", &ANCHORS);
    update_val(jsondata, "CLASS_NAMES", &CLASS_NAMES);
    update_val(jsondata, "MODEL_PATH", &MODEL_PATH);
    update_val(jsondata, "STRIDES", &STRIDES);

    update_val(jsondata, "MAX_MASK_OBJ_COUNT", &MAX_MASK_OBJ_COUNT);
    MAX_MASK_OBJ_COUNT = MIN(MAX_MASK_OBJ_COUNT, SAMPLE_MAX_BBOX_COUNT);
    update_val(jsondata, "MAX_SUB_INFER_COUNT", &MAX_SUB_INFER_COUNT);
    MAX_SUB_INFER_COUNT = MIN(MAX_SUB_INFER_COUNT, SAMPLE_MAX_BBOX_COUNT);
    update_val(jsondata, "FACE_FEAT_LEN", &FACE_FEAT_LEN);

    // update_val(jsondata, "USE_WARP_PREPROCESS", &use_warp_preprocess);
    update_val(jsondata, "OSD_DRAW_NAME", &b_draw_obj_name);

    std::string strModelType;
    m_model_type = (MODEL_TYPE_E)get_model_type(&jsondata, strModelType);
    ALOGI("load model %s", MODEL_PATH.c_str());
    m_runner.reset((ax_runner_base *)OBJFactory::getInstance().getObjectByID(m_runner_type));
    if (!m_runner.get())
    {
        ALOGE("runner instantiate failed!");
        return -1;
    }
    int ret = m_runner->init(MODEL_PATH.c_str());
    if (ret)
    {
        ALOGE("runner init load model failed!");
        return ret;
    }

    int unknown_cls_count = MAX(0, CLASS_NUM - CLASS_NAMES.size());
    for (int i = 0; i < unknown_cls_count; i++)
    {
        CLASS_NAMES.push_back("unknown");
    }
    return 0;
}

void ax_model_single_base_t::deinit()
{
    m_runner->deinit();
    if (bMalloc)
    {
        ax_sys_memfree(dstFrame.pPhy, dstFrame.pVir);
    }
    disbale_track();
}

int ax_model_single_base_t::preprocess(axdl_image_t *srcFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    memcpy(&dstFrame, srcFrame, sizeof(axdl_image_t));
    bMalloc = false;
    return 0;
}

int ax_model_single_base_t::inference(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    int ret = preprocess(pstFrame, crop_resize_box, results);
    if (ret != 0)
    {
        ALOGE("preprocess failed %d", ret);
        return ret;
    }
    ret = m_runner->inference(&dstFrame, crop_resize_box);
    if (ret != 0)
    {
        ALOGE("inference failed 0x%x", ret);
        return ret;
    }
    ret = post_process(pstFrame, crop_resize_box, results);
    results->bObjTrack = b_track ? 1 : 0;
    if (b_track)
    {
        tracker_objs.n_objects = results->nObjSize > TRACK_OBJETCS_MAX_SIZE ? TRACK_OBJETCS_MAX_SIZE : results->nObjSize;
        for (int i = 0; i < tracker_objs.n_objects; i++)
        {
            tracker_objs.objects[i].rect.x = results->mObjects[i].bbox.x;
            tracker_objs.objects[i].rect.y = results->mObjects[i].bbox.y;
            tracker_objs.objects[i].rect.width = results->mObjects[i].bbox.w;
            tracker_objs.objects[i].rect.height = results->mObjects[i].bbox.h;
            tracker_objs.objects[i].label = results->mObjects[i].label;
            tracker_objs.objects[i].prob = results->mObjects[i].prob;
            tracker_objs.objects[i].user_data = &results->mObjects[i];
        }

        bytetracker_track(tracker, &tracker_objs);
        // printf("%d %d \n", tracker_objs.n_objects, tracker_objs.n_track_objects);
        results->nObjSize = tracker_objs.n_track_objects > SAMPLE_MAX_BBOX_COUNT ? SAMPLE_MAX_BBOX_COUNT : tracker_objs.n_track_objects;
        for (int i = 0; i < results->nObjSize; i++)
        {
            results->mObjects[i].bbox.x = tracker_objs.track_objects[i].rect.x;
            results->mObjects[i].bbox.y = tracker_objs.track_objects[i].rect.y;
            results->mObjects[i].bbox.w = tracker_objs.track_objects[i].rect.width;
            results->mObjects[i].bbox.h = tracker_objs.track_objects[i].rect.height;
            results->mObjects[i].label = tracker_objs.track_objects[i].label;
            results->mObjects[i].prob = tracker_objs.track_objects[i].prob;
            results->mObjects[i].track_id = tracker_objs.track_objects[i].track_id;
            axdl_object_t *obj = (axdl_object_t *)tracker_objs.objects[i].user_data;
            memcpy(results->mObjects[i].objname, obj->objname, sizeof(results->mObjects[i].objname));
            if (obj->bHasBoxVertices)
            {
                results->mObjects[i].bHasBoxVertices = obj->bHasBoxVertices;
                memcpy(results->mObjects[i].bbox_vertices, obj->bbox_vertices, sizeof(results->mObjects[i].bbox_vertices));
            }
            if (obj->nLandmark)
            {
                results->mObjects[i].nLandmark = obj->nLandmark;
                results->mObjects[i].landmark = obj->landmark;
            }
            if (obj->bHasMask)
            {
                results->mObjects[i].bHasMask = obj->bHasMask;
                memcpy(&results->mObjects[i].mYolov5Mask, &obj->mYolov5Mask, sizeof(results->mObjects[i].mYolov5Mask));
            }
            if (obj->bHasFaceFeat)
            {
                results->mObjects[i].bHasFaceFeat = obj->bHasFaceFeat;
                memcpy(&results->mObjects[i].mFaceFeat, &obj->mFaceFeat, sizeof(results->mObjects[i].mFaceFeat));
            }
        }
    }

    return ret;
}

void ax_model_multi_base_t::enable_track(int frame_rate, int track_buffer)
{
    model_0->enable_track(frame_rate, track_buffer);
    b_track = true;
}

void ax_model_multi_base_t::disbale_track()
{
    model_0->disbale_track();
    b_track = false;
}

int ax_model_multi_base_t::init(void *json_obj)
{
    auto jsondata = *(nlohmann::json *)json_obj;

    std::string strModelType;
    m_model_type = (MODEL_TYPE_E)get_model_type(&jsondata, strModelType);

    switch (m_model_type)
    {
    case MT_MLM_HUMAN_POSE_AXPPL:
        model_1.reset(new ax_model_pose_axppl_sub);
        break;
    case MT_MLM_HUMAN_POSE_HRNET:
        model_1.reset(new ax_model_pose_hrnet_sub);
        break;
    case MT_MLM_ANIMAL_POSE_HRNET:
        model_1.reset(new ax_model_pose_hrnet_animal_sub);
        break;
    case MT_MLM_HAND_POSE:
        model_1.reset(new ax_model_pose_hand_sub);
        break;
    case MT_MLM_FACE_RECOGNITION:
        model_1.reset(new ax_model_face_feat_extactor_sub);
        break;
    case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
        model_1.reset(new ax_model_license_plate_recognition_sub);
        break;
    default:
        ALOGE("not multi level model type %d", (int)m_model_type);
        return -1;
    }

    if (jsondata.contains("MODEL_MAJOR") && jsondata.contains("MODEL_MINOR"))
    {
        nlohmann::json json_major = jsondata["MODEL_MAJOR"];

        std::string strModelType;
        int mt = get_model_type(&json_major, strModelType);
        model_0.reset((ax_model_base *)OBJFactory::getInstance().getObjectByID(mt));
        model_0->init((void *)&json_major);

        nlohmann::json json_minor = jsondata["MODEL_MINOR"];

        update_val(json_minor, "CLASS_ID", &CLASS_IDS);

        if (json_minor.contains("FACE_DATABASE"))
        {
            nlohmann::json database = json_minor["FACE_DATABASE"];
            for (nlohmann::json::iterator it = database.begin(); it != database.end(); ++it)
            {
                ALOGI("name:%s path:%s", it.key().c_str(), it.value().get<std::string>().c_str());
                ax_model_faceid faceid;
                faceid.path = it.value();
                faceid.name = it.key();
                face_register_ids.push_back(faceid);
            }
        }
        model_1->init((void *)&json_minor);

        update_val(json_minor, "FACE_RECOGNITION_THRESHOLD", &FACE_RECOGNITION_THRESHOLD);
        update_val(jsondata, "MAX_MASK_OBJ_COUNT", &MAX_MASK_OBJ_COUNT);
        MAX_MASK_OBJ_COUNT = MIN(MAX_MASK_OBJ_COUNT, SAMPLE_MAX_BBOX_COUNT);
        update_val(jsondata, "MAX_SUB_INFER_COUNT", &MAX_SUB_INFER_COUNT);
        MAX_SUB_INFER_COUNT = MIN(MAX_SUB_INFER_COUNT, SAMPLE_MAX_BBOX_COUNT);
        update_val(jsondata, "FACE_FEAT_LEN", &FACE_FEAT_LEN);
        update_val(jsondata, "OSD_DRAW_NAME", &b_draw_obj_name);

        model_0->set_face_recognition_threshold(FACE_RECOGNITION_THRESHOLD);
        model_0->set_max_mask_obj_count(MAX_MASK_OBJ_COUNT);
        model_0->set_sub_infer_count(MAX_SUB_INFER_COUNT);
        model_0->set_face_feat_len(FACE_FEAT_LEN);

        model_1->set_face_recognition_threshold(FACE_RECOGNITION_THRESHOLD);
        model_1->set_max_mask_obj_count(MAX_MASK_OBJ_COUNT);
        model_1->set_sub_infer_count(MAX_SUB_INFER_COUNT);
        model_1->set_face_feat_len(FACE_FEAT_LEN);
    }
    else
        return -1;
    return 0;
}

void ax_model_multi_base_t::deinit()
{
    model_1->deinit();
    model_0->deinit();
}