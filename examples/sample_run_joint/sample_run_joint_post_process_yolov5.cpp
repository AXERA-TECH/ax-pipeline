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

#include <vector>
#include <string.h>
#include "fstream"

#include "sample_run_joint_post_process.h"
#include "detection.hpp"

#include "../utilities/json.hpp"

#include "joint.h"
#include "../utilities/sample_log.h"

float YOLOV5_PROB_THRESHOLD = 0.4f;
float YOLOV5_NMS_THRESHOLD = 0.45f;
int YOLOV5_CLASS_NUM = 80;

std::vector<float> YOLOV5_ANCHORS = {10, 13, 16, 30,
                                     33, 23, 30, 61,
                                     62, 45, 59, 119,
                                     116, 90, 156, 198,
                                     373, 326};

std::vector<std::string> YOLOV5_CLASS_NAMES = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"};

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

int sample_parse_param_yolov5(char *json_file_path)
{
    std::ifstream f(json_file_path);
    if (f.fail())
    {
        ALOGE("%s doesn`t exist,generate it by default param\n", json_file_path);
        nlohmann::json json_data;
        json_data["YOLOV5_PROB_THRESHOLD"] = YOLOV5_PROB_THRESHOLD;
        json_data["YOLOV5_NMS_THRESHOLD"] = YOLOV5_NMS_THRESHOLD;
        json_data["YOLOV5_CLASS_NUM"] = YOLOV5_CLASS_NUM;
        json_data["YOLOV5_ANCHORS"] = YOLOV5_ANCHORS;
        json_data["YOLOV5_CLASS_NAMES"] = YOLOV5_CLASS_NAMES;

        std::string json_ctx = json_data.dump(4);
        std::ofstream of(json_file_path);
        of << json_ctx;
        of.close();
        return -1;
    }

    auto jsondata = nlohmann::json::parse(f);

    update_val(jsondata, "YOLOV5_PROB_THRESHOLD", &YOLOV5_PROB_THRESHOLD);
    update_val(jsondata, "YOLOV5_NMS_THRESHOLD", &YOLOV5_NMS_THRESHOLD);
    update_val(jsondata, "YOLOV5_CLASS_NUM", &YOLOV5_CLASS_NUM);
    update_val(jsondata, "YOLOV5_ANCHORS", &YOLOV5_ANCHORS);
    update_val(jsondata, "YOLOV5_CLASS_NAMES", &YOLOV5_CLASS_NAMES);

    if (YOLOV5_ANCHORS.size() != 18)
    {
        ALOGE("ANCHORS SIZE MUST BE 18\n");
        return -1;
    }

    if (YOLOV5_CLASS_NUM != YOLOV5_CLASS_NAMES.size())
    {
        ALOGE("YOLOV5_CLASS_NUM != YOLOV5_CLASS_NAMES SIZE(%d:%d)\n", YOLOV5_CLASS_NUM, YOLOV5_CLASS_NAMES.size());
        return -1;
    }
    return 0;
}
/// @brief 模型后处理函数
/// @param nOutputSize 输出的节点数
/// @param pOutputsInfo 输出的节点对应的信息，包含维度信息、节点名称等
/// @param pOutputs 输出的节点的数据指针，包含物理地址、虚拟地址等
/// @param pResults 目标检测的结果信息
/// @param SAMPLE_ALGO_WIDTH 算法的输入宽
/// @param SAMPLE_ALGO_HEIGHT 算法的输入高
/// @param SAMPLE_MAJOR_STREAM_WIDTH 相机图像的宽
/// @param SAMPLE_MAJOR_STREAM_HEIGHT 相机图像的高
void sample_run_joint_post_process_yolov5(AX_U32 nOutputSize, AX_JOINT_IOMETA_T *pOutputsInfo, AX_JOINT_IO_BUFFER_T *pOutputs, sample_run_joint_results *pResults,
                                   int SAMPLE_ALGO_WIDTH, int SAMPLE_ALGO_HEIGHT, int SAMPLE_MAJOR_STREAM_WIDTH, int SAMPLE_MAJOR_STREAM_HEIGHT)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / YOLOV5_PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        if (output.pShape[3] == (YOLOV5_CLASS_NUM + 5) * 3)
        {
            generate_proposals_yolov5(stride, ptr, YOLOV5_PROB_THRESHOLD, proposals, SAMPLE_ALGO_WIDTH, SAMPLE_ALGO_HEIGHT, YOLOV5_ANCHORS.data(), prob_threshold_unsigmoid, YOLOV5_CLASS_NUM);
        }
        else if (YOLOV5_CLASS_NUM == 1 && output.pShape[3] == (YOLOV5_CLASS_NUM + 10 + 5) * 3)
        {
            generate_proposals_yolov5_face(stride, ptr, YOLOV5_PROB_THRESHOLD, proposals, SAMPLE_ALGO_WIDTH, SAMPLE_ALGO_HEIGHT, YOLOV5_ANCHORS.data(), prob_threshold_unsigmoid);
        }
        else
        {
            ALOGE("[YOLOV5] - (YOLOV5_CLASS_NUM + 5) * 3 should equal %d,but YOLOV5_CLASS_NUM got %d\n", output.pShape[3], YOLOV5_CLASS_NUM);
            ALOGE("[YOLOV5_FACE] - (YOLOV5_CLASS_NUM + 10 + 5) * 3 should equal %d,but YOLOV5_CLASS_NUM got %d\n", output.pShape[3], YOLOV5_CLASS_NUM);
            break;
        }
    }

    detection::get_out_bbox(proposals, objects, YOLOV5_NMS_THRESHOLD, SAMPLE_ALGO_HEIGHT, SAMPLE_ALGO_WIDTH, SAMPLE_MAJOR_STREAM_HEIGHT, SAMPLE_MAJOR_STREAM_WIDTH);
    pResults->size = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < pResults->size; i++)
    {
        const detection::Object &obj = objects[i];
        pResults->objects[i].bbox.x = obj.rect.x;
        pResults->objects[i].bbox.y = obj.rect.y;
        pResults->objects[i].bbox.w = obj.rect.width;
        pResults->objects[i].bbox.h = obj.rect.height;
        pResults->objects[i].label = obj.label;
        pResults->objects[i].prob = obj.prob;

        if (obj.label < YOLOV5_CLASS_NAMES.size())
        {
            strcpy(pResults->objects[i].objname, YOLOV5_CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(pResults->objects[i].objname, "unknown");
        }
    }
}