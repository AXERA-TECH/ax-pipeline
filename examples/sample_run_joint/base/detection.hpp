/*
 * AXERA is pleased to support the open source community by making ax-samples available.
 *
 * Copyright (c) 2022, AXERA Semiconductor (Shanghai) Co., Ltd. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */

/*
 * Author: ZHEQIUSHUI
 */

#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <opencv2/opencv.hpp>

namespace detection
{
    typedef struct
    {
        int grid0;
        int grid1;
        int stride;
    } GridAndStride;

    typedef struct
    {
        cv::Rect_<float> rect;
        int label;
        float prob;
        cv::Point2f landmark[5];
        /* for yolov5-seg */
        cv::Mat mask;
        std::vector<float> mask_feat;
    } Object;

    /* for palm hand detection */
    typedef struct PalmObject
    {
        cv::Rect_<float> rect;
        float prob;
        cv::Point2f vertices[4];
        cv::Point2f landmarks[7];
        cv::Mat affine_trans_mat;
        cv::Mat affine_trans_mat_inv;
    } PalmObject;

    static int softmax(const float *src, float *dst, int length)
    {
        const float max_value = *std::max_element(src, src + length);
        float denominator{0};

        for (int i = 0; i < length; ++i)
        {
            dst[i] = std::exp /*fast_exp*/ (src[i] - max_value);
            denominator += dst[i];
        }

        for (int i = 0; i < length; ++i)
        {
            dst[i] /= denominator;
        }

        return 0;
    }

    static inline float sigmoid(float x)
    {
        return static_cast<float>(1.f / (1.f + exp(-x)));
    }

    template <typename T>
    static inline float intersection_area(const T &a, const T &b)
    {
        cv::Rect_<float> inter = a.rect & b.rect;
        return inter.area();
    }

    template <typename T>
    static void qsort_descent_inplace(std::vector<T> &faceobjects, int left, int right)
    {
        int i = left;
        int j = right;
        float p = faceobjects[(left + right) / 2].prob;

        while (i <= j)
        {
            while (faceobjects[i].prob > p)
                i++;

            while (faceobjects[j].prob < p)
                j--;

            if (i <= j)
            {
                // swap
                std::swap(faceobjects[i], faceobjects[j]);

                i++;
                j--;
            }
        }
#pragma omp parallel sections
        {
#pragma omp section
            {
                if (left < j)
                    qsort_descent_inplace(faceobjects, left, j);
            }
#pragma omp section
            {
                if (i < right)
                    qsort_descent_inplace(faceobjects, i, right);
            }
        }
    }

    template <typename T>
    static void qsort_descent_inplace(std::vector<T> &faceobjects)
    {
        if (faceobjects.empty())
            return;

        qsort_descent_inplace(faceobjects, 0, faceobjects.size() - 1);
    }

    template <typename T>
    static void nms_sorted_bboxes(const std::vector<T> &faceobjects, std::vector<int> &picked, float nms_threshold)
    {
        picked.clear();

        const int n = faceobjects.size();

        std::vector<float> areas(n);
        for (int i = 0; i < n; i++)
        {
            areas[i] = faceobjects[i].rect.area();
        }

        for (int i = 0; i < n; i++)
        {
            const T &a = faceobjects[i];

            int keep = 1;
            for (int j = 0; j < (int)picked.size(); j++)
            {
                const T &b = faceobjects[picked[j]];

                // intersection over union
                float inter_area = intersection_area(a, b);
                float union_area = areas[i] + areas[picked[j]] - inter_area;
                // float IoU = inter_area / union_area
                if (inter_area / union_area > nms_threshold)
                    keep = 0;
            }

            if (keep)
                picked.push_back(i);
        }
    }

    static void get_out_bbox(std::vector<Object> &proposals, std::vector<Object> &objects, const float nms_threshold, int letterbox_rows, int letterbox_cols, int src_rows, int src_cols)
    {
        qsort_descent_inplace(proposals);
        std::vector<int> picked;
        nms_sorted_bboxes(proposals, picked, nms_threshold);

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

        int count = picked.size();

        objects.resize(count);
        for (int i = 0; i < count; i++)
        {
            objects[i] = proposals[picked[i]];
            float x0 = (objects[i].rect.x);
            float y0 = (objects[i].rect.y);
            float x1 = (objects[i].rect.x + objects[i].rect.width);
            float y1 = (objects[i].rect.y + objects[i].rect.height);

            x0 = (x0 - tmp_w) * ratio_x;
            y0 = (y0 - tmp_h) * ratio_y;
            x1 = (x1 - tmp_w) * ratio_x;
            y1 = (y1 - tmp_h) * ratio_y;

            for (int l = 0; l < 5; l++)
            {
                auto lx = objects[i].landmark[l].x;
                auto ly = objects[i].landmark[l].y;
                objects[i].landmark[l] = cv::Point2f((lx - tmp_w) * ratio_x, (ly - tmp_h) * ratio_y);
            }

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

    static void generate_proposals_yolov5(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects,
                                          int letterbox_cols, int letterbox_rows, const float *anchors, float prob_threshold_unsigmoid, int cls_num)
    {
        int anchor_num = 3;
        int feat_w = letterbox_cols / stride;
        int feat_h = letterbox_rows / stride;
        int anchor_group;
        if (stride == 8)
            anchor_group = 1;
        if (stride == 16)
            anchor_group = 2;
        if (stride == 32)
            anchor_group = 3;

        auto feature_ptr = feat;

        for (int h = 0; h <= feat_h - 1; h++)
        {
            for (int w = 0; w <= feat_w - 1; w++)
            {
                for (int a = 0; a <= anchor_num - 1; a++)
                {
                    if (feature_ptr[4] < prob_threshold_unsigmoid)
                    {
                        feature_ptr += (cls_num + 5);
                        continue;
                    }

                    // process cls score
                    int class_index = 0;
                    float class_score = -FLT_MAX;
                    for (int s = 0; s <= cls_num - 1; s++)
                    {
                        float score = feature_ptr[s + 5];
                        if (score > class_score)
                        {
                            class_index = s;
                            class_score = score;
                        }
                    }
                    // process box score
                    float box_score = feature_ptr[4];
                    float final_score = sigmoid(box_score) * sigmoid(class_score);

                    if (final_score >= prob_threshold)
                    {
                        float dx = sigmoid(feature_ptr[0]);
                        float dy = sigmoid(feature_ptr[1]);
                        float dw = sigmoid(feature_ptr[2]);
                        float dh = sigmoid(feature_ptr[3]);
                        float pred_cx = (dx * 2.0f - 0.5f + w) * stride;
                        float pred_cy = (dy * 2.0f - 0.5f + h) * stride;
                        float anchor_w = anchors[(anchor_group - 1) * 6 + a * 2 + 0];
                        float anchor_h = anchors[(anchor_group - 1) * 6 + a * 2 + 1];
                        float pred_w = dw * dw * 4.0f * anchor_w;
                        float pred_h = dh * dh * 4.0f * anchor_h;
                        float x0 = pred_cx - pred_w * 0.5f;
                        float y0 = pred_cy - pred_h * 0.5f;
                        float x1 = pred_cx + pred_w * 0.5f;
                        float y1 = pred_cy + pred_h * 0.5f;

                        Object obj;
                        obj.rect.x = x0;
                        obj.rect.y = y0;
                        obj.rect.width = x1 - x0;
                        obj.rect.height = y1 - y0;
                        obj.label = class_index;
                        obj.prob = final_score;
                        objects.push_back(obj);
                    }

                    feature_ptr += (cls_num + 5);
                }
            }
        }
    }

    static void generate_proposals_yolov5_face(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects,
                                               int letterbox_cols, int letterbox_rows, const float *anchors, float prob_threshold_unsigmoid)
    {
        int anchor_num = 3;
        int feat_w = letterbox_cols / stride;
        int feat_h = letterbox_rows / stride;
        int cls_num = 1;
        int anchor_group;
        if (stride == 8)
            anchor_group = 1;
        if (stride == 16)
            anchor_group = 2;
        if (stride == 32)
            anchor_group = 3;

        auto feature_ptr = feat;

        for (int h = 0; h <= feat_h - 1; h++)
        {
            for (int w = 0; w <= feat_w - 1; w++)
            {
                for (int a = 0; a <= anchor_num - 1; a++)
                {
                    if (feature_ptr[4] < prob_threshold_unsigmoid)
                    {
                        feature_ptr += (cls_num + 5 + 10);
                        continue;
                    }

                    // process cls score
                    int class_index = 0;
                    float class_score = -FLT_MAX;
                    for (int s = 0; s <= cls_num - 1; s++)
                    {
                        float score = feature_ptr[s + 5 + 10];
                        if (score > class_score)
                        {
                            class_index = s;
                            class_score = score;
                        }
                    }
                    // process box score
                    float box_score = feature_ptr[4];
                    float final_score = sigmoid(box_score) * sigmoid(class_score);

                    if (final_score >= prob_threshold)
                    {
                        float dx = sigmoid(feature_ptr[0]);
                        float dy = sigmoid(feature_ptr[1]);
                        float dw = sigmoid(feature_ptr[2]);
                        float dh = sigmoid(feature_ptr[3]);
                        float pred_cx = (dx * 2.0f - 0.5f + w) * stride;
                        float pred_cy = (dy * 2.0f - 0.5f + h) * stride;
                        float anchor_w = anchors[(anchor_group - 1) * 6 + a * 2 + 0];
                        float anchor_h = anchors[(anchor_group - 1) * 6 + a * 2 + 1];
                        float pred_w = dw * dw * 4.0f * anchor_w;
                        float pred_h = dh * dh * 4.0f * anchor_h;
                        float x0 = pred_cx - pred_w * 0.5f;
                        float y0 = pred_cy - pred_h * 0.5f;
                        float x1 = pred_cx + pred_w * 0.5f;
                        float y1 = pred_cy + pred_h * 0.5f;

                        Object obj;
                        obj.rect.x = x0;
                        obj.rect.y = y0;
                        obj.rect.width = x1 - x0;
                        obj.rect.height = y1 - y0;
                        obj.label = class_index;
                        obj.prob = final_score;

                        const float *landmark_ptr = feature_ptr + 5;
                        for (int l = 0; l < 5; l++)
                        {
                            float lx = landmark_ptr[l * 2 + 0];
                            float ly = landmark_ptr[l * 2 + 1];
                            lx = lx * anchor_w + w * stride;
                            ly = ly * anchor_h + h * stride;
                            obj.landmark[l] = cv::Point2f(lx, ly);
                        }

                        objects.push_back(obj);
                    }

                    feature_ptr += (cls_num + 5 + 10);
                }
            }
        }
    }

    static void generate_proposals_yolox(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects,
                                         int letterbox_cols, int letterbox_rows, int cls_num = 80)
    {
        int feat_w = letterbox_cols / stride;
        int feat_h = letterbox_rows / stride;

        auto feat_ptr = feat;

        for (int h = 0; h <= feat_h - 1; h++)
        {
            for (int w = 0; w <= feat_w - 1; w++)
            {
                float box_objectness = feat_ptr[4];
                if (box_objectness < prob_threshold)
                {
                    feat_ptr += 85;
                    continue;
                }

                // process cls score
                int class_index = 0;
                float class_score = -FLT_MAX;
                for (int s = 0; s <= cls_num - 1; s++)
                {
                    float score = feat_ptr[s + 5];
                    if (score > class_score)
                    {
                        class_index = s;
                        class_score = score;
                    }
                }

                float box_prob = box_objectness * class_score;

                if (box_prob > prob_threshold)
                {
                    float x_center = (feat_ptr[0] + w) * stride;
                    float y_center = (feat_ptr[1] + h) * stride;
                    float w = exp(feat_ptr[2]) * stride;
                    float h = exp(feat_ptr[3]) * stride;
                    float x0 = x_center - w * 0.5f;
                    float y0 = y_center - h * 0.5f;

                    Object obj;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = w;
                    obj.rect.height = h;
                    obj.label = class_index;
                    obj.prob = box_prob;

                    objects.push_back(obj);
                }

                feat_ptr += 85;
            }
        }
    }

    static void generate_proposals_yolov7(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects,
                                          int letterbox_cols, int letterbox_rows, const float *anchors, int cls_num = 80)
    {
        int feat_w = letterbox_cols / stride;
        int feat_h = letterbox_rows / stride;

        auto feat_ptr = feat;

        for (int h = 0; h <= feat_h - 1; h++)
        {
            for (int w = 0; w <= feat_w - 1; w++)
            {
                for (int a_index = 0; a_index < 3; ++a_index)
                {
                    float box_objectness = feat_ptr[4];
                    if (box_objectness < prob_threshold)
                    {
                        feat_ptr += 85;
                        continue;
                    }

                    // process cls score
                    int class_index = 0;
                    float class_score = -FLT_MAX;
                    for (int s = 0; s <= cls_num - 1; s++)
                    {
                        float score = feat_ptr[s + 5];
                        if (score > class_score)
                        {
                            class_index = s;
                            class_score = score;
                        }
                    }

                    float box_prob = box_objectness * class_score;

                    if (box_prob > prob_threshold)
                    {
                        float x_center = (feat_ptr[0] * 2 - 0.5f + (float)w) * (float)stride;
                        float y_center = (feat_ptr[1] * 2 - 0.5f + (float)h) * (float)stride;
                        float box_w = (feat_ptr[2] * 2) * (feat_ptr[2] * 2) * anchors[a_index * 2];
                        float box_h = (feat_ptr[3] * 2) * (feat_ptr[3] * 2) * anchors[a_index * 2 + 1];
                        float x0 = x_center - box_w * 0.5f;
                        float y0 = y_center - box_h * 0.5f;

                        Object obj;
                        obj.rect.x = x0;
                        obj.rect.y = y0;
                        obj.rect.width = box_w;
                        obj.rect.height = box_h;
                        obj.label = class_index;
                        obj.prob = box_prob;

                        objects.push_back(obj);
                    }

                    feat_ptr += 85;
                }
            }
        }
    }

    static void generate_proposals_nanodet(const float *pred_80_32_nhwc, int stride, const int &model_w,
                                           const int &model_h, float prob_threshold, std::vector<Object> &objects, int num_class = 80)
    {
        const int num_grid_x = model_w / stride;
        const int num_grid_y = model_h / stride;
        // Discrete distribution parameter, see the following resources for more details:
        // [nanodet-m.yml](https://github.com/RangiLyu/nanodet/blob/main/config/nanodet-m.yml)
        // [GFL](https://arxiv.org/pdf/2006.04388.pdf)
        const int reg_max_1 = 8; // 32 / 4;
        const int channel = num_class + reg_max_1 * 4;

        for (int i = 0; i < num_grid_y; i++)
        {
            for (int j = 0; j < num_grid_x; j++)
            {
                const int idx = i * num_grid_x + j;

                const float *scores = pred_80_32_nhwc + idx * channel;

                // find label with max score
                int label = -1;
                float score = -FLT_MAX;
                for (int k = 0; k < num_class; k++)
                {
                    if (scores[k] > score)
                    {
                        label = k;
                        score = scores[k];
                    }
                }
                score = sigmoid(score);

                if (score >= prob_threshold)
                {
                    float pred_ltrb[4];
                    for (int k = 0; k < 4; k++)
                    {
                        float dis = 0.f;
                        // predicted distance distribution after softmax
                        float dis_after_sm[8] = {0.};
                        softmax(scores + num_class + k * reg_max_1, dis_after_sm, 8);

                        // integral on predicted discrete distribution
                        for (int l = 0; l < reg_max_1; l++)
                        {
                            dis += l * dis_after_sm[l];
                            // printf("%2.6f ", dis_after_sm[l]);
                        }
                        // printf("\n");

                        pred_ltrb[k] = dis * stride;
                    }

                    // predict box center point
                    float pb_cx = (j + 0.5f) * stride;
                    float pb_cy = (i + 0.5f) * stride;

                    float x0 = pb_cx - pred_ltrb[0]; // left
                    float y0 = pb_cy - pred_ltrb[1]; // top
                    float x1 = pb_cx + pred_ltrb[2]; // right
                    float y1 = pb_cy + pred_ltrb[3]; // bottom

                    Object obj;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = x1 - x0;
                    obj.rect.height = y1 - y0;
                    obj.label = label;
                    obj.prob = score;

                    objects.push_back(obj);
                }
            }
        }
    }

    static void generate_grids_and_stride(const int target_w, const int target_h, std::vector<int> &strides, std::vector<GridAndStride> &grid_strides)
    {
        for (auto stride : strides)
        {
            int num_grid_w = target_w / stride;
            int num_grid_h = target_h / stride;
            for (int g1 = 0; g1 < num_grid_h; g1++)
            {
                for (int g0 = 0; g0 < num_grid_w; g0++)
                {
                    GridAndStride gs;
                    gs.grid0 = g0;
                    gs.grid1 = g1;
                    gs.stride = stride;
                    grid_strides.push_back(gs);
                }
            }
        }
    }

    static void generate_yolox_proposals(std::vector<GridAndStride> grid_strides, float *feat_ptr, float prob_threshold, std::vector<Object> &objects, int wxc)
    {
        const int num_grid = 3549;
        const int num_class = 1;
        const int num_anchors = grid_strides.size();

        for (int anchor_idx = 0; anchor_idx < num_anchors; anchor_idx++)
        {

            float box_objectness = feat_ptr[4 * wxc + anchor_idx];
            float box_cls_score = feat_ptr[5 * wxc + anchor_idx];
            float box_prob = box_objectness * box_cls_score;
            if (box_prob > prob_threshold)
            {
                Object obj;
                // printf("%d,%d\n",num_anchors,anchor_idx);
                const int grid0 = grid_strides[anchor_idx].grid0;   // 0
                const int grid1 = grid_strides[anchor_idx].grid1;   // 0
                const int stride = grid_strides[anchor_idx].stride; // 8
                // yolox/models/yolo_head.py decode logic
                //  outputs[..., :2] = (outputs[..., :2] + grids) * strides
                //  outputs[..., 2:4] = torch.exp(outputs[..., 2:4]) * strides
                float x_center = (feat_ptr[0 + anchor_idx] + grid0) * stride;
                float y_center = (feat_ptr[1 * wxc + anchor_idx] + grid1) * stride;
                float w = exp(feat_ptr[2 * wxc + anchor_idx]) * stride;
                float h = exp(feat_ptr[3 * wxc + anchor_idx]) * stride;
                float x0 = x_center - w * 0.5f;
                float y0 = y_center - h * 0.5f;
                obj.rect.x = x0;
                obj.rect.y = y0;
                obj.rect.width = w;
                obj.rect.height = h;
                obj.label = 0;
                obj.prob = box_prob;

                objects.push_back(obj);
            }
        } // point anchor loop
    }

    static void generate_proposals_yolov5_seg(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects,
                                              int letterbox_cols, int letterbox_rows, const float *anchors, float prob_threshold_unsigmoid, int cls_num = 80, int mask_proto_dim = 32)
    {
        int anchor_num = 3;
        int feat_w = letterbox_cols / stride;
        int feat_h = letterbox_rows / stride;
        int anchor_group;
        if (stride == 8)
            anchor_group = 1;
        if (stride == 16)
            anchor_group = 2;
        if (stride == 32)
            anchor_group = 3;

        auto feature_ptr = feat;

        for (int h = 0; h <= feat_h - 1; h++)
        {
            for (int w = 0; w <= feat_w - 1; w++)
            {
                for (int a = 0; a <= anchor_num - 1; a++)
                {
                    if (feature_ptr[4] < prob_threshold_unsigmoid)
                    {
                        feature_ptr += (cls_num + 5 + mask_proto_dim);
                        continue;
                    }

                    // process cls score
                    int class_index = 0;
                    float class_score = -FLT_MAX;
                    for (int s = 0; s <= cls_num - 1; s++)
                    {
                        float score = feature_ptr[s + 5];
                        if (score > class_score)
                        {
                            class_index = s;
                            class_score = score;
                        }
                    }
                    // process box score
                    float box_score = feature_ptr[4];
                    float final_score = sigmoid(box_score) * sigmoid(class_score);

                    if (final_score >= prob_threshold)
                    {
                        float dx = sigmoid(feature_ptr[0]);
                        float dy = sigmoid(feature_ptr[1]);
                        float dw = sigmoid(feature_ptr[2]);
                        float dh = sigmoid(feature_ptr[3]);
                        float pred_cx = (dx * 2.0f - 0.5f + w) * stride;
                        float pred_cy = (dy * 2.0f - 0.5f + h) * stride;
                        float anchor_w = anchors[(anchor_group - 1) * 6 + a * 2 + 0];
                        float anchor_h = anchors[(anchor_group - 1) * 6 + a * 2 + 1];
                        float pred_w = dw * dw * 4.0f * anchor_w;
                        float pred_h = dh * dh * 4.0f * anchor_h;
                        float x0 = pred_cx - pred_w * 0.5f;
                        float y0 = pred_cy - pred_h * 0.5f;
                        float x1 = pred_cx + pred_w * 0.5f;
                        float y1 = pred_cy + pred_h * 0.5f;

                        Object obj;
                        obj.rect.x = x0;
                        obj.rect.y = y0;
                        obj.rect.width = x1 - x0;
                        obj.rect.height = y1 - y0;
                        obj.label = class_index;
                        obj.prob = final_score;
                        obj.mask_feat.resize(mask_proto_dim);
                        for (int k = 0; k < mask_proto_dim; k++)
                        {
                            obj.mask_feat[k] = feature_ptr[cls_num + 5 + k];
                        }
                        objects.push_back(obj);
                    }

                    feature_ptr += (cls_num + 5 + mask_proto_dim);
                }
            }
        }
    }

    static void get_out_bbox_mask(std::vector<Object> &proposals, std::vector<Object> &objects, int objs_max_count, const float *mask_proto, int mask_proto_dim, int mask_stride, const float nms_threshold, int letterbox_rows, int letterbox_cols, int src_rows, int src_cols)
    {
        qsort_descent_inplace(proposals);
        std::vector<int> picked;
        nms_sorted_bboxes(proposals, picked, nms_threshold);

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

        int mask_proto_h = int(letterbox_rows / mask_stride);
        int mask_proto_w = int(letterbox_cols / mask_stride);

        int count = std::min(objs_max_count, (int)picked.size());
        objects.resize(count);

        for (int i = 0; i < count; i++)
        {
            objects[i] = proposals[picked[i]];
            float x0 = (objects[i].rect.x);
            float y0 = (objects[i].rect.y);
            float x1 = (objects[i].rect.x + objects[i].rect.width);
            float y1 = (objects[i].rect.y + objects[i].rect.height);
            /* naive RoiAlign by opencv */
            int hstart = std::floor(objects[i].rect.y / mask_stride);
            int hend = std::ceil(objects[i].rect.y / mask_stride + objects[i].rect.height / mask_stride);
            int wstart = std::floor(objects[i].rect.x / mask_stride);
            int wend = std::ceil(objects[i].rect.x / mask_stride + objects[i].rect.width / mask_stride);

            hstart = std::min(std::max(hstart, 0), mask_proto_h);
            wstart = std::min(std::max(wstart, 0), mask_proto_w);
            hend = std::min(std::max(hend, 0), mask_proto_h);
            wend = std::min(std::max(wend, 0), mask_proto_w);

            int mask_w = wend - wstart;
            int mask_h = hend - hstart;

            cv::Mat mask = cv::Mat(mask_h, mask_w, CV_32FC1);
            if (mask_w > 0 && mask_h > 0)
            {
                std::vector<cv::Range> roi_ranges;
                roi_ranges.push_back(cv::Range(0, 1));
                roi_ranges.push_back(cv::Range::all());
                roi_ranges.push_back(cv::Range(hstart, hend));
                roi_ranges.push_back(cv::Range(wstart, wend));

                cv::Mat mask_protos = cv::Mat(mask_proto_dim, mask_proto_h * mask_proto_w, CV_32FC1, (float *)mask_proto);
                int sz[] = {1, mask_proto_dim, mask_proto_h, mask_proto_w};
                cv::Mat mask_protos_reshape = mask_protos.reshape(1, 4, sz);
                cv::Mat protos = mask_protos_reshape(roi_ranges).clone().reshape(0, {mask_proto_dim, mask_w * mask_h});
                cv::Mat mask_proposals = cv::Mat(1, mask_proto_dim, CV_32FC1, (float *)objects[i].mask_feat.data());
                cv::Mat masks_feature = (mask_proposals * protos);
                /* sigmoid */
                cv::exp(-masks_feature.reshape(1, {mask_h, mask_w}), mask);
                mask = 1.0 / (1.0 + mask);
            }

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
            // cv::resize(mask, mask, cv::Size((int)objects[i].rect.width, (int)objects[i].rect.height));
            objects[i].mask = mask > 0.5;
        }
    }

    static void generate_proposals_palm(std::vector<PalmObject> &region_list, float score_thresh, int input_img_w, int input_img_h, float *scores_ptr, float *bboxes_ptr, int head_count, const int *strides, const int *anchor_size, const float *anchor_offset, const int *feature_map_size, float prob_threshold_unsigmoid)
    {
        int idx = 0;
        for (int i = 0; i < head_count; i++)
        {
            for (int y = 0; y < feature_map_size[i]; y++)
            {
                for (int x = 0; x < feature_map_size[i]; x++)
                {
                    for (int k = 0; k < anchor_size[i]; k++)
                    {
                        if (scores_ptr[idx] < prob_threshold_unsigmoid)
                        {
                            idx++;
                            continue;
                        }

                        const float x_center = (x + anchor_offset[i]) * 1.0f / feature_map_size[i];
                        const float y_center = (y + anchor_offset[i]) * 1.0f / feature_map_size[i];
                        float score = sigmoid(scores_ptr[idx]);

                        if (score > score_thresh)
                        {
                            float *p = bboxes_ptr + (idx * 18);

                            float cx = p[0] / input_img_w + x_center;
                            float cy = p[1] / input_img_h + y_center;
                            float w = p[2] / input_img_w;
                            float h = p[3] / input_img_h;

                            float x0 = cx - w * 0.5f;
                            float y0 = cy - h * 0.5f;
                            float x1 = cx + w * 0.5f;
                            float y1 = cy + h * 0.5f;

                            PalmObject region;
                            region.prob = score;
                            region.rect.x = x0;
                            region.rect.y = y0;
                            region.rect.width = x1 - x0;
                            region.rect.height = y1 - y0;

                            for (int j = 0; j < 7; j++)
                            {
                                float lx = p[4 + (2 * j) + 0];
                                float ly = p[4 + (2 * j) + 1];
                                lx += x_center * input_img_w;
                                ly += y_center * input_img_h;
                                lx /= (float)input_img_w;
                                ly /= (float)input_img_h;

                                region.landmarks[j].x = lx;
                                region.landmarks[j].y = ly;
                            }
                            region_list.push_back(region);
                        }
                        idx++;
                    }
                }
            }
        }
    }

    static void transform_rects_palm(PalmObject &object)
    {
        float x0 = object.landmarks[0].x;
        float y0 = object.landmarks[0].y;
        float x1 = object.landmarks[2].x;
        float y1 = object.landmarks[2].y;
        float rotation = M_PI * 0.5f - std::atan2(-(y1 - y0), x1 - x0);

        float hand_cx;
        float hand_cy;
        float shift_x = 0.0f;
        float shift_y = -0.5f;
        if (rotation == 0)
        {
            hand_cx = object.rect.x + object.rect.width * 0.5f + (object.rect.width * shift_x);
            hand_cy = object.rect.y + object.rect.height * 0.5f + (object.rect.height * shift_y);
        }
        else
        {
            float dx = (object.rect.width * shift_x) * std::cos(rotation) -
                       (object.rect.height * shift_y) * std::sin(rotation);
            float dy = (object.rect.width * shift_x) * std::sin(rotation) +
                       (object.rect.height * shift_y) * std::cos(rotation);
            hand_cx = object.rect.x + object.rect.width * 0.5f + dx;
            hand_cy = object.rect.y + object.rect.height * 0.5f + dy;
        }

        float long_side = (std::max)(object.rect.width, object.rect.height);
        float dx = long_side * 1.3f;
        float dy = long_side * 1.3f;

        object.vertices[0].x = -dx;
        object.vertices[0].y = -dy;
        object.vertices[1].x = +dx;
        object.vertices[1].y = -dy;
        object.vertices[2].x = +dx;
        object.vertices[2].y = +dy;
        object.vertices[3].x = -dx;
        object.vertices[3].y = +dy;

        for (int i = 0; i < 4; i++)
        {
            float sx = object.vertices[i].x;
            float sy = object.vertices[i].y;
            object.vertices[i].x = sx * std::cos(rotation) - sy * std::sin(rotation);
            object.vertices[i].y = sx * std::sin(rotation) + sy * std::cos(rotation);
            object.vertices[i].x += hand_cx;
            object.vertices[i].y += hand_cy;
        }
    }

    static void get_out_bbox_palm(std::vector<PalmObject> &proposals, std::vector<PalmObject> &objects, const float nms_threshold, int letterbox_rows, int letterbox_cols, int src_rows, int src_cols)
    {
        qsort_descent_inplace(proposals);
        std::vector<int> picked;
        nms_sorted_bboxes(proposals, picked, nms_threshold);

        int count = picked.size();
        objects.resize(count);
        for (int i = 0; i < count; i++)
        {
            objects[i] = proposals[picked[i]];
            transform_rects_palm(objects[i]);
        }

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

        float ratio_x = (float)src_cols / resize_cols;
        float ratio_y = (float)src_rows / resize_rows;

        for (auto &object : objects)
        {
            for (auto &vertice : object.vertices)
            {
                vertice.x = (vertice.x * letterbox_cols - tmp_w) * ratio_x;
                vertice.y = (vertice.y * letterbox_rows - tmp_h) * ratio_y;
            }

            for (auto &ld : object.landmarks)
            {
                ld.x = (ld.x * letterbox_cols - tmp_w) * ratio_x;
                ld.y = (ld.y * letterbox_rows - tmp_h) * ratio_y;
            }
            // get warpaffine transform mat to landmark detect
            // cv::Point2f src_pts[4];
            // src_pts[0] = object.vertices[0];
            // src_pts[1] = object.vertices[1];
            // src_pts[2] = object.vertices[2];
            // src_pts[3] = object.vertices[3];

            // cv::Point2f dst_pts[4];
            // dst_pts[0] = cv::Point2f(0, 0);
            // dst_pts[1] = cv::Point2f(224, 0);
            // dst_pts[2] = cv::Point2f(224, 224);
            // dst_pts[3] = cv::Point2f(0, 224);

            // object.affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
            // cv::invertAffineTransform(object.affine_trans_mat, object.affine_trans_mat_inv);
        }
    }

} // namespace detection
