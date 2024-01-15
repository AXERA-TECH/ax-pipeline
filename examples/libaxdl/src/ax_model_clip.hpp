#pragma once
#include "../include/ax_model_base.hpp"
#include "utilities/object_register.hpp"
#include "../../utilities/ringbuffer.hpp"
#include "base/detection.hpp"

class ax_model_clip : public ax_model_single_base_t
{
    std::vector<std::vector<float>> texts_feature;
    std::vector<std::string> texts;

    int len_image_feature = 0;

    static void softmax(const std::vector<std::vector<float>> &input, std::vector<std::vector<float>> &result)
    {
        result.reserve(input.size());

        for (const auto &row : input)
        {
            std::vector<float> rowResult;
            rowResult.reserve(row.size());

            float maxVal = *std::max_element(row.begin(), row.end());

            float sumExp = 0.0;
            for (float val : row)
            {
                float expVal = std::exp(val - maxVal);
                rowResult.emplace_back(expVal);
                sumExp += expVal;
            }

            for (float &val : rowResult)
            {
                val /= sumExp;
            }

            result.emplace_back(std::move(rowResult));
        }
    }

    static void forward(
        const std::vector<std::vector<float>> &imageFeatures, const std::vector<std::vector<float>> &textFeatures,
        std::vector<std::vector<float>> &logits_per_image, std::vector<std::vector<float>> &logits_per_text)
    {
        std::vector<std::vector<float>> logitsPerImage;
        logitsPerImage.reserve(imageFeatures.size());

        for (const auto &_row : imageFeatures)
        {
            float norm = 0.0;
            for (float val : _row)
            {
                norm += val * val;
            }
            norm = std::sqrt(norm);
            std::vector<float> normRow;
            normRow.reserve(_row.size());
            for (float val : _row)
            {
                normRow.push_back(val / norm);
            }

            std::vector<float> row;
            row.reserve(textFeatures.size());
            for (const auto &textRow : textFeatures)
            {
                float sum = 0.0;
                for (size_t i = 0; i < normRow.size(); i++)
                {
                    sum += normRow[i] * textRow[i];
                }
                row.push_back(100 * sum);
            }
            logitsPerImage.push_back(std::move(row));
        }

        std::vector<std::vector<float>> logitsPerText(logitsPerImage[0].size(), std::vector<float>(logitsPerImage.size()));

        for (size_t i = 0; i < logitsPerImage.size(); i++)
        {
            for (size_t j = 0; j < logitsPerImage[i].size(); j++)
            {
                logitsPerText[j][i] = logitsPerImage[i][j];
            }
        }

        softmax(logitsPerImage, logits_per_image);
        softmax(logitsPerText, logits_per_text);
    }

protected:
    // 在这里添加自定义属性
    int sub_init(void *json_obj) override;
    int preprocess(axdl_image_t *srcFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
    void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
    void draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness) override;
};
REGISTER(MT_CLIP, ax_model_clip)

#ifdef BUILT_WITH_ONNXRUNTIME
#include "onnxruntime_cxx_api.h"
class ax_model_owlvit : public ax_model_single_base_t
{
    struct ax_model_owlvit_texts
    {
        std::vector<int64_t> input_ids;
        std::vector<float> text_feature;
        std::string text;
    };
    
    std::vector<ax_model_owlvit_texts> texts;

    std::vector<cv::Rect2f> pred_boxes;

    int len_image_feature = 0;
    int len_text_feature = 512;
    int len_text_token = 16;
    int len_pred_bbox = 576;

    std::string device{"cpu"};
    Ort::Env env;
    Ort::SessionOptions session_options;
    std::shared_ptr<Ort::Session> DecoderSession;
    Ort::MemoryInfo memory_info_handler = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    int num_thread = 4;

    const char
        *DecoderInputNames[3]{"image_embeds", "/owlvit/Div_output_0", "input_ids"},
        *DecoderOutputNames[1]{"logits"};

    std::vector<int64_t> image_features_shape = {1, 24, 24, 768};
    std::vector<int64_t> text_features_shape = {1, 512};

    void get_out_bbox(std::vector<detection::Object> &objects, int letterbox_rows, int letterbox_cols, int src_rows, int src_cols)
    {
        /* yolov5 draw the result */
        float scale_letterbox;
        int resize_rows;
        int resize_cols;
        if ((letterbox_rows * 1.0 / src_rows) < (letterbox_cols * 1.0 / src_cols))
        {
            scale_letterbox = letterbox_rows * 1.0 / src_rows;
        }
        else
        {
            scale_letterbox = letterbox_cols * 1.0 / src_cols;
        }
        resize_cols = int(scale_letterbox * src_cols);
        resize_rows = int(scale_letterbox * src_rows);

        int tmp_h = (letterbox_rows - resize_rows) / 2;
        int tmp_w = (letterbox_cols - resize_cols) / 2;

        float ratio_x = (float)src_rows / resize_rows;
        float ratio_y = (float)src_cols / resize_cols;

        int count = objects.size();

        for (int i = 0; i < count; i++)
        {
            float x0 = (objects[i].rect.x);
            float y0 = (objects[i].rect.y);
            float x1 = (objects[i].rect.x + objects[i].rect.width);
            float y1 = (objects[i].rect.y + objects[i].rect.height);

            x0 = (x0 - tmp_w) * ratio_x;
            y0 = (y0 - tmp_h) * ratio_y;
            x1 = (x1 - tmp_w) * ratio_x;
            y1 = (y1 - tmp_h) * ratio_y;

            x0 = std::max(std::min(x0, (float)(src_cols - 1)), 0.f);
            y0 = std::max(std::min(y0, (float)(src_rows - 1)), 0.f);
            x1 = std::max(std::min(x1, (float)(src_cols - 1)), 0.f);
            y1 = std::max(std::min(y1, (float)(src_rows - 1)), 0.f);

            objects[i].rect.x = x0;
            objects[i].rect.y = y0;
            objects[i].rect.width = x1 - x0;
            objects[i].rect.height = y1 - y0;
        }
    }

protected:
    int sub_init(void *json_obj) override;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_OWLVIT, ax_model_owlvit)
#endif