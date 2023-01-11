#include "../include/c_api.h"

#include "../../utilities/sample_log.h"
#include "../include/ax_model_base.hpp"
#include "../../utilities/json.hpp"
#include "utilities/object_register.hpp"

#include "fstream"

extern "C"
{
    // 给sipeed的python包用的
    typedef int (*result_callback_for_sipeed_py)(void *, libaxdl_results_t *);
    result_callback_for_sipeed_py g_cb_results_sipeed_py = NULL;
    int register_result_callback(result_callback_for_sipeed_py cb)
    {
        g_cb_results_sipeed_py = cb;
        return 0;
    }

    // 给sipeed的python包用的
    typedef int (*display_callback_for_sipeed_py)(int, int, int, char **);
    display_callback_for_sipeed_py g_cb_display_sipeed_py = NULL;
    int register_display_callback(display_callback_for_sipeed_py cb)
    {
        g_cb_display_sipeed_py = cb;
        return 0;
    }
}

struct ax_model_handle_t
{
    std::shared_ptr<ax_model_base> model = nullptr;
};

int libaxdl_parse_param_init(char *json_file_path, void **pModels)
{
    OBJFactory::getInstance().print_obj();
    std::ifstream f(json_file_path);
    if (f.fail())
    {
        return -1;
    }
    auto jsondata = nlohmann::json::parse(f);
    f.close();

    std::string strModelType;
    int mt = ax_model_base::get_model_type(&jsondata, strModelType);
    if (mt == MT_UNKNOWN)
    {
        return -1;
    }
    *pModels = new ax_model_handle_t;
    ax_model_base *model = (ax_model_base *)OBJFactory::getInstance().getObjectByID(mt);
    if (model == nullptr)
    {
        ALOGE("create model failed mt=%d", mt);
        return -1;
    }

    ((ax_model_handle_t *)(*pModels))->model.reset(model);
    return ((ax_model_handle_t *)(*pModels))->model->init(&jsondata);
}

void libaxdl_deinit(void **pModels)
{
    if (pModels && (ax_model_handle_t *)(*pModels) && ((ax_model_handle_t *)(*pModels))->model.get())
    {
        ((ax_model_handle_t *)(*pModels))->model->deinit();
        delete (ax_model_handle_t *)(*pModels);
        *pModels = nullptr;
    }
}

int libaxdl_get_ivps_width_height(void *pModels, char *json_file_path, int *width_ivps, int *height_ivps)
{
    std::ifstream f(json_file_path);
    if (f.fail())
    {
        return -1;
    }
    auto jsondata = nlohmann::json::parse(f);
    f.close();

    if (jsondata.contains("SAMPLE_IVPS_ALGO_WIDTH") && jsondata.contains("SAMPLE_IVPS_ALGO_HEIGHT"))
    {
        *width_ivps = jsondata["SAMPLE_IVPS_ALGO_WIDTH"];
        *height_ivps = jsondata["SAMPLE_IVPS_ALGO_HEIGHT"];
        ((ax_model_handle_t *)pModels)->model->set_det_restore_resolution(*width_ivps, *height_ivps);
    }
    else
    {
        switch (((ax_model_handle_t *)pModels)->model->get_model_type())
        {
        case MT_MLM_HUMAN_POSE_AXPPL:
        case MT_MLM_HUMAN_POSE_HRNET:
        case MT_MLM_ANIMAL_POSE_HRNET:
        case MT_MLM_HAND_POSE:
        case MT_MLM_FACE_RECOGNITION:
        case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
            *width_ivps = 960;
            *height_ivps = 540;
            ((ax_model_handle_t *)pModels)->model->set_det_restore_resolution(*width_ivps, *height_ivps);
            break;
        default:
            *width_ivps = ((ax_model_handle_t *)pModels)->model->get_algo_width();
            *height_ivps = ((ax_model_handle_t *)pModels)->model->get_algo_height();
            break;
        }
    }
    return 0;
}
int libaxdl_get_color_space(void *pModels)
{
    return ((ax_model_handle_t *)pModels)->model->get_color_space();
}
int libaxdl_get_model_type(void *pModels)
{
    return ((ax_model_handle_t *)pModels)->model->get_model_type();
}

int libaxdl_inference(void *pModels, const void *pstFrame, libaxdl_results_t *pResults)
{
    pResults->mModelType = ((ax_model_handle_t *)pModels)->model->get_model_type();
    int ret = ((ax_model_handle_t *)pModels)->model->inference(pstFrame, nullptr, pResults);
    if (ret)
        return ret;
    int width, height;
    ((ax_model_handle_t *)pModels)->model->get_det_restore_resolution(width, height);
    for (int i = 0; i < pResults->nObjSize; i++)
    {
        pResults->mObjects[i].bbox.x /= width;
        pResults->mObjects[i].bbox.y /= height;
        pResults->mObjects[i].bbox.w /= width;
        pResults->mObjects[i].bbox.h /= height;

        for (int j = 0; j < pResults->mObjects[i].nLandmark; j++)
        {
            pResults->mObjects[i].landmark[j].x /= width;
            pResults->mObjects[i].landmark[j].y /= height;
        }

        if (pResults->mObjects[i].bHasBoxVertices)
        {
            for (size_t j = 0; j < 4; j++)
            {
                pResults->mObjects[i].bbox_vertices[j].x /= width;
                pResults->mObjects[i].bbox_vertices[j].y /= height;
            }
        }
    }

    if (g_cb_results_sipeed_py)
    {
        ret = g_cb_results_sipeed_py((void *)pstFrame, pResults);
    }

    {
        static int fcnt = 0;
        static int fps = -1;
        fcnt++;
        static struct timespec ts1, ts2;
        clock_gettime(CLOCK_MONOTONIC, &ts2);
        if ((ts2.tv_sec * 1000 + ts2.tv_nsec / 1000000) - (ts1.tv_sec * 1000 + ts1.tv_nsec / 1000000) >= 1000)
        {
            fps = fcnt;
            ts1 = ts2;
            fcnt = 0;
        }
        pResults->niFps = fps;
    }
    return 0;
}

int libaxdl_draw_results(void *pModels, libaxdl_canvas_t *canvas, libaxdl_results_t *pResults, float fontscale, int thickness, int offset_x, int offset_y)
{
    if (g_cb_display_sipeed_py && (g_cb_display_sipeed_py(canvas->height, canvas->width, CV_8UC4, (char **)&canvas->data) != 0)) {
        return 0; // python will disable show
    }

    cv::Mat image(canvas->height, canvas->width, CV_8UC4, canvas->data);

    ((ax_model_handle_t *)pModels)->model->draw_results(image, pResults, fontscale, thickness, offset_x, offset_y);
    return 0;
}