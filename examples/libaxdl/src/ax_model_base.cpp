#include "../include/ax_model_base.hpp"
#include "../../utilities/json.hpp"

#include "utilities/object_register.hpp"
#include "../../utilities/sample_log.h"
#include "ax_sys_api.h"
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
            m_model_type = (MODEL_TYPE_E)mt;
        }
        else if (jsondata["MODEL_TYPE"].is_string())
        {
            strModelType = jsondata["MODEL_TYPE"];

            auto item = ModelTypeTable.find(strModelType);

            if (item != ModelTypeTable.end())
            {
                m_model_type = (MODEL_TYPE_E)ModelTypeTable[strModelType];
            }
            else
            {
                m_model_type = MT_UNKNOWN;
            }
        }
    }
    return m_model_type;
}

void ax_model_base::draw_bbox(cv::Mat &image, libaxdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
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
        label_size = cv::getTextSize(results->mObjects[i].objname, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);
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

        cv::putText(image, results->mObjects[i].objname, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale,
                    cv::Scalar(0, 0, 0, 255), thickness);
    }
}

void ax_model_base::draw_fps(cv::Mat &image, libaxdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    sprintf(fps_info, "fps:%02d", results->niFps);
    cv::Size label_size = cv::getTextSize(fps_info, cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5, thickness * 2, NULL);
    cv::putText(image, fps_info, cv::Point(0, label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5,
                cv::Scalar(255, 0, 255, 0), thickness * 2);
}

int ax_model_single_base_t::init(void *json_obj)
{
    auto jsondata = *(nlohmann::json *)json_obj;

    update_val(jsondata, "PROB_THRESHOLD", &PROB_THRESHOLD);
    update_val(jsondata, "NMS_THRESHOLD", &NMS_THRESHOLD);
    update_val(jsondata, "CLASS_NUM", &CLASS_NUM);
    update_val(jsondata, "ANCHORS", &ANCHORS);
    update_val(jsondata, "CLASS_NAMES", &CLASS_NAMES);
    update_val(jsondata, "MODEL_PATH", &m_model_path);

    std::string strModelType;
    m_model_type = (MODEL_TYPE_E)get_model_type(&jsondata, strModelType);
    ALOGI("load model %s", m_model_path.c_str());
    m_runner.reset(new ax_joint_runner_ax620);
    m_runner->init(m_model_path.c_str());

    if (ANCHORS.size() != 18)
    {
        ALOGE("ANCHORS SIZE MUST BE 18\n");
        return -1;
    }

    if (CLASS_NUM != CLASS_NAMES.size())
    {
        ALOGE("CLASS_NUM != CLASS_NAMES SIZE(%d:%d)\n", CLASS_NUM, CLASS_NAMES.size());
        return -1;
    }
    return 0;
}

void ax_model_single_base_t::deinit()
{
    m_runner->deinit();
    if (bMalloc)
    {
        AX_SYS_MemFree(dstFrame.pPhy, dstFrame.pVir);
    }
}

int ax_model_single_base_t::preprocess(const void *srcFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    memcpy(&dstFrame, srcFrame, sizeof(AX_NPU_CV_Image));
    bMalloc = false;
    return 0;
}

int ax_model_single_base_t::inference(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
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
        ALOGE("inference failed %d", ret);
        return ret;
    }
    ret = post_process(pstFrame, crop_resize_box, results);
    return ret;
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
        update_val(json_minor, "FACE_RECOGNITION_THRESHOLD", &FACE_RECOGNITION_THRESHOLD);

        model_1->init((void *)&json_minor);
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