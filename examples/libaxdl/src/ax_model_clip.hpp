#include "../include/ax_model_base.hpp"
#include "utilities/object_register.hpp"
#include "../../utilities/ringbuffer.hpp"

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