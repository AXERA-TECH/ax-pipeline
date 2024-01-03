#include "ax_model_clip.hpp"
#include "../../utilities/sample_log.h"
#include "../../utilities/json.hpp"
#include "utilities/file.hpp"
#include "opencv2/opencv.hpp"

int ax_model_clip::sub_init(void *json_obj)
{
    auto jsondata = *(nlohmann::json *)json_obj;
    if (jsondata.contains("TEXTS"))
    {
        nlohmann::json database = jsondata["TEXTS"];
        for (nlohmann::json::iterator it = database.begin(); it != database.end(); ++it)
        {
            ALOGI("name:%s path:%s", it.key().c_str(), it.value().get<std::string>().c_str());
            std::string path = it.value();
            std::string name = it.key();

            if (!utilities::file_exist(path))
            {
                ALOGE("text feature file not exist: %s", path.c_str());
                continue;
            }
            std::vector<char> data;
            if (!utilities::read_file(path, data))
            {
                ALOGE("read text feature file failed: %s", path.c_str());
                continue;
            }
            std::vector<float> feature(data.size() / sizeof(float));
            memcpy(feature.data(), data.data(), data.size());

            texts.push_back(name);
            texts_feature.push_back(feature);
        }
    }
    else
    {
        ALOGE("json data not contain TEXTS");
        return -1;
    }
    return 0;
}

int ax_model_clip::preprocess(axdl_image_t *srcFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (!dstFrame.pVir)
    {
        dstFrame.eDtype = axdl_color_space_rgb;
        dstFrame.nHeight = get_algo_height();
        dstFrame.nWidth = get_algo_width();
        dstFrame.tStride_W = dstFrame.nWidth;
        if (dstFrame.eDtype == axdl_color_space_nv12)
        {
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3 / 2;
        }
        else if (dstFrame.eDtype == axdl_color_space_rgb || dstFrame.eDtype == axdl_color_space_bgr)
        {
            dstFrame.eDtype = axdl_color_space_rgb;
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3;
        }
        else
        {
            ALOGE("just only support nv12/rgb/bgr format\n");
            return -1;
        }
        ax_sys_memalloc(&dstFrame.pPhy, (void **)&dstFrame.pVir, dstFrame.nSize, 32, NULL);
        bMalloc = true;
    }
    cv::Mat dst(dstFrame.nHeight, dstFrame.nWidth, CV_8UC3, (unsigned char *)dstFrame.pVir);
    cv::Mat src(srcFrame->nHeight, srcFrame->nWidth, CV_8UC3, (unsigned char *)srcFrame->pVir);
    cv::Mat src_rgb;

    if (srcFrame->eDtype == axdl_color_space_bgr)
    {
        src_rgb = src;
    }
    else if (srcFrame->eDtype == axdl_color_space_rgb)
    {
        cv::cvtColor(src, src_rgb, cv::COLOR_RGB2BGR);
    }
    else if (srcFrame->eDtype == axdl_color_space_nv12)
    {
        src = cv::Mat(srcFrame->nHeight * 1.5, srcFrame->nWidth, CV_8UC1, (unsigned char *)srcFrame->pVir);
        cv::cvtColor(src, src_rgb, cv::COLOR_YUV2RGB_NV12);
    }
    else if (srcFrame->eDtype == axdl_color_space_nv21)
    {
        src = cv::Mat(srcFrame->nHeight * 1.5, srcFrame->nWidth, CV_8UC1, (unsigned char *)srcFrame->pVir);
        cv::cvtColor(src, src_rgb, cv::COLOR_YUV2RGB_NV21);
    }
    cv::resize(src_rgb, dst, cv::Size(get_algo_width(), get_algo_height()));

    // cv::imwrite("image.png", dst);

    return 0;
}

int ax_model_clip::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();
    if (len_image_feature == 0)
    {
        len_image_feature = 1;
        for (size_t i = 0; i < pOutputsInfo->vShape.size(); i++)
        {
            len_image_feature *= pOutputsInfo->vShape[i];
        }
        ALOGI("clip image feature len: %d", len_image_feature);
    }

    std::vector<std::vector<float>> image_feaure(1);
    image_feaure[0].resize(len_image_feature);
    memcpy(image_feaure[0].data(), pOutputsInfo[0].pVirAddr, len_image_feature * sizeof(float));
    std::vector<std::vector<float>> logits_per_image, logits_per_text;
    forward(image_feaure, texts_feature, logits_per_image, logits_per_text);

    results->nObjSize = texts.size();
    for (int i = 0; i < results->nObjSize; i++)
    {
        results->mObjects[i].prob = logits_per_image[0][i];
    }
    return 0;
}

void ax_model_clip::draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    char text[256];
    int x = 50, y = 50;
    cv::Size label_size;
    int baseLine = 0;
    for (int i = 0; i < results->nObjSize; i++)
    {
        sprintf(text, "%s %.2f", texts[i].c_str(), results->mObjects[i].prob);
        label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);

        cv::rectangle(image, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                      cv::Scalar(255, 255, 255, 255), -1);

        cv::putText(image, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale,
                    cv::Scalar(0, 0, 0, 255), thickness);

        y += label_size.height + 10;
    }
}

void ax_model_clip::draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness)
{
    char text[256];
    float x = 50, y = 50;

    // find the max id and score
    int maxid = 0;
    float max_score = -FLT_MAX;
    for (int i = 0; i < results->nObjSize; i++)
    {
        if (results->mObjects[i].prob > max_score)
        {
            max_score = results->mObjects[i].prob;
            maxid = i;
        }
    }

    for (int i = 0; i < results->nObjSize; i++)
    {
        sprintf(text, "%10s %.2f", texts[i].c_str(), results->mObjects[i].prob);
        if (i == maxid)
        {
            m_drawers[chn].add_text(text, {x / m_drawers[chn].get_width(), y / m_drawers[chn].get_height()}, {255, 0, 255, 0}, fontscale * 2, thickness);
        }
        else
        {
            m_drawers[chn].add_text(text, {x / m_drawers[chn].get_width(), y / m_drawers[chn].get_height()}, {255, 0, 0, 255}, fontscale * 2, thickness);
        }
        y += 50;
    }
}

#ifdef BUILT_WITH_ONNXRUNTIME
int ax_model_owlvit::sub_init(void *json_obj)
{
    auto jsondata = *(nlohmann::json *)json_obj;
    if (jsondata.contains("TEXTS"))
    {
        std::vector<nlohmann::json> database = jsondata["TEXTS"];
        for (size_t i = 0; i < database.size(); i++)
        {
            if (database[i].contains("text") &&
                database[i].contains("texts_feature") &&
                database[i].contains("input_ids"))
            {
                std::string text = database[i]["text"];
                std::string path = database[i]["texts_feature"];
                std::vector<int64_t> input_id = database[i]["input_ids"];
                if ((int)input_id.size() != len_text_token)
                {
                    ALOGE("input_ids size not match: %s", path.c_str());
                    continue;
                }
                if (!utilities::file_exist(path))
                {
                    ALOGE("text feature file not exist: %s", path.c_str());
                    continue;
                }

                std::vector<char> data;
                if (!utilities::read_file(path, data))
                {
                    ALOGE("read text feature file failed: %s", path.c_str());
                    continue;
                }
                if (data.size() != len_text_feature * sizeof(float))
                {
                    ALOGE("text feature size not match: %s", path.c_str());
                    continue;
                }
                std::vector<float> feature(data.size() / sizeof(float));
                memcpy(feature.data(), data.data(), data.size());
                texts.push_back({input_id, feature, text});
            }
        }
    }
    else
    {
        ALOGE("json data not contain TEXTS");
        return -1;
    }
    if (jsondata.contains("DECODER_PATH"))
    {
        std::string decoder_path = jsondata["DECODER_PATH"];
        DecoderSession.reset(new Ort::Session(env, decoder_path.c_str(), session_options));
    }
    else
    {
        ALOGE("json data not contain DECODER_PATH");
        return -1;
    }

    return 0;
}

int ax_model_owlvit::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();
    if (len_image_feature == 0)
    {
        len_image_feature = 1;
        for (size_t i = 0; i < pOutputsInfo[1].vShape.size(); i++)
        {
            len_image_feature *= pOutputsInfo[1].vShape[i];
        }
        ALOGI("clip image feature len: %d", len_image_feature);

        pred_boxes.resize(len_pred_bbox);
    }
    auto output = (float *)pOutputsInfo[0].pVirAddr;

    for (int i = 0; i < len_pred_bbox; i++)
    {
        float xc = output[i * 4 + 0] * get_algo_width();
        float yc = output[i * 4 + 1] * get_algo_height();
        pred_boxes[i].width = output[i * 4 + 2] * get_algo_width();
        pred_boxes[i].height = output[i * 4 + 3] * get_algo_height();
        pred_boxes[i].x = xc - pred_boxes[i].width / 2;
        pred_boxes[i].y = yc - pred_boxes[i].height / 2;
    }

    float prob_threshold_u_sigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);

    std::vector<detection::Object> objects;
    std::vector<int64_t> input_ids_shape = {1, len_text_token};
    for (size_t i = 0; i < texts.size(); i++)
    {
        std::vector<Ort::Value> inputTensors;

        inputTensors.push_back(Ort::Value::CreateTensor<float>(
            memory_info_handler, (float *)pOutputsInfo[1].pVirAddr, len_image_feature, image_features_shape.data(), image_features_shape.size()));
        inputTensors.push_back(Ort::Value::CreateTensor<float>(
            memory_info_handler, texts[i].text_feature.data(), len_text_feature, text_features_shape.data(), text_features_shape.size()));

        // for (size_t j = 0; j < input_ids.size(); j++)
        // {
        //     printf("%d ", input_ids[j]);
        // }
        // printf("\n");

        inputTensors.push_back((Ort::Value::CreateTensor<int64_t>(
            memory_info_handler, texts[i].input_ids.data(), len_text_token, input_ids_shape.data(), input_ids_shape.size())));

        Ort::RunOptions runOptions;
        auto DecoderOutputTensors = DecoderSession->Run(runOptions, DecoderInputNames, inputTensors.data(),
                                                        inputTensors.size(), DecoderOutputNames, 1);

        auto &logits_output = DecoderOutputTensors[0];
        auto logits_ptr = logits_output.GetTensorMutableData<float>();
        auto logits_shape = logits_output.GetTensorTypeAndShapeInfo().GetShape();

        int logits_size = 1;
        for (size_t j = 0; j < logits_shape.size(); j++)
        {
            logits_size *= logits_shape[j];
        }

        for (int j = 0; j < logits_size; j++)
        {
            // float score = sigmoid(logits[j]);
            // printf("%f %f\n", score, logits[j]);
            if (logits_ptr[j] > prob_threshold_u_sigmoid)
            {
                detection::Object obj;
                obj.text = texts[i].text;
                obj.prob = detection::sigmoid(logits_ptr[j]);
                obj.rect = pred_boxes[j];
                obj.label = i;
                objects.push_back(obj);
            }
        }
    }

    get_out_bbox(objects, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;

        if (obj.label < (int)texts.size())
        {
            strcpy(results->mObjects[i].objname, obj.text.c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }

    return 0;
}
#endif