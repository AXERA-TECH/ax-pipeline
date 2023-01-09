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

int ax_model_base_t::get_model_type(void *json_obj, std::string &strModelType)
{
    SAMPLE_RUN_JOINT_MODEL_TYPE m_model_type = MT_UNKNOWN;
    auto jsondata = *(nlohmann::json *)json_obj;
    if (jsondata.contains("MODEL_TYPE"))
    {
        if (jsondata["MODEL_TYPE"].is_number_integer())
        {
            int mt = -1;
            mt = jsondata["MODEL_TYPE"];
            m_model_type = (SAMPLE_RUN_JOINT_MODEL_TYPE)mt;
        }
        else if (jsondata["MODEL_TYPE"].is_string())
        {
            strModelType = jsondata["MODEL_TYPE"];

            auto item = ModelTypeTable.find(strModelType);

            if (item != ModelTypeTable.end())
            {
                m_model_type = (SAMPLE_RUN_JOINT_MODEL_TYPE)ModelTypeTable[strModelType];
            }
            else
            {
                m_model_type = MT_UNKNOWN;
            }
        }
    }
    return m_model_type;
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
    m_model_type = (SAMPLE_RUN_JOINT_MODEL_TYPE)get_model_type(&jsondata, strModelType);
    ALOGI("load model %s", m_model_path.c_str());
    m_runner.init(m_model_path.c_str());

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
    m_runner.deinit();
    if (bMalloc)
    {
        AX_SYS_MemFree(dstFrame.pPhy, dstFrame.pVir);
    }
}

int ax_model_single_base_t::preprocess(const void *srcFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    memcpy(&dstFrame, srcFrame, sizeof(AX_NPU_CV_Image));
    bMalloc = false;
    return 0;
}

int ax_model_single_base_t::inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    int ret = preprocess(pstFrame, crop_resize_box, results);
    if (ret != 0)
    {
        ALOGE("preprocess failed %d", ret);
        return ret;
    }
    ret = m_runner.inference(&dstFrame, crop_resize_box);
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
    m_model_type = (SAMPLE_RUN_JOINT_MODEL_TYPE)get_model_type(&jsondata, strModelType);

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
        model_0.reset((ax_model_base_t *)OBJFactory::getInstance().getObjectByID(mt));
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