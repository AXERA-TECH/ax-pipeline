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

#include "opencv2/opencv.hpp"
#include "../sample_run_joint/sample_run_joint_post_process.h"
#include "../sample_run_joint/pose.hpp"
#include "osd_utils.h"
#include "../common/sample_def.h"
#include "sstream"

void genImg(int charlen, float fontscale, int thickness, osd_utils_img *out)
{
    std::stringstream ss;

    for (int i = 0; i < charlen; i++)
    {
        ss << "O";
    }
    std::string text = ss.str();

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);

    out->width = label_size.width;
    out->height = label_size.height * 1.5;
    out->channel = 4;
    out->data = new uchar[out->width * out->height * out->channel];
    memset(out->data, 0, out->width * out->height * out->channel);
}

int putText(char *text, float fontscale, int thickness, osd_utils_img *base, osd_utils_img *out)
{
    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);
    out->width = label_size.width;
    out->height = label_size.height * 1.5;
    out->channel = 4;

    if (out->width * out->height * out->channel > base->width * base->height * base->channel)
    {
        return -1;
    }
    out->data = base->data;

    memset(out->data, 0, out->width * out->height * out->channel);

    cv::Mat src(out->height, out->width, CV_8UC4, out->data);
    cv::putText(src, text, cv::Point(0, label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale, cv::Scalar(255, 0, 0, 255), 2);
    return 0;
}

void releaseImg(osd_utils_img *img)
{
    if (img)
    {
        delete[] img->data;
        memset(img, 0, sizeof(osd_utils_img));
    }
}

static inline void draw_pose_result(cv::Mat &img, sample_run_joint_object *pObj, int joints_num, int offset_x, int offset_y)
{
    for (int i = 0; i < joints_num; i++)
    {
        cv::circle(img, cv::Point(pObj->pose_landmark[i].x * img.cols + offset_x, pObj->pose_landmark[i].y * img.rows + offset_y), 4, cv::Scalar(0, 255, 0), cv::FILLED);
    }

    cv::Scalar color;
    cv::Point pt1;
    cv::Point pt2;
    static std::vector<pose::skeleton> pairs = {{15, 13, 0},
                                                {13, 11, 0},
                                                {16, 14, 0},
                                                {14, 12, 0},
                                                {11, 12, 0},
                                                {5, 11, 0},
                                                {6, 12, 0},
                                                {5, 6, 0},
                                                {5, 7, 0},
                                                {6, 8, 0},
                                                {7, 9, 0},
                                                {8, 10, 0},
                                                {1, 2, 0},
                                                {0, 1, 0},
                                                {0, 2, 0},
                                                {1, 3, 0},
                                                {2, 4, 0},
                                                {0, 5, 0},
                                                {0, 6, 0}};
    for (auto &element : pairs)
    {
        switch (element.left_right_neutral)
        {
        case 0:
            color = cv::Scalar(255, 0, 0);
            break;
        case 1:
            color = cv::Scalar(0, 0, 255);
            break;
        default:
            color = cv::Scalar(0, 255, 0);
        }

        int x1 = (int)(pObj->pose_landmark[element.connection[0]].x * img.cols) + offset_x;
        int y1 = (int)(pObj->pose_landmark[element.connection[0]].y * img.rows) + offset_y;
        int x2 = (int)(pObj->pose_landmark[element.connection[1]].x * img.cols) + offset_x;
        int y2 = (int)(pObj->pose_landmark[element.connection[1]].y* img.rows) + offset_y;

        x1 = std::max(std::min(x1, (img.cols - 1)), 0);
        y1 = std::max(std::min(y1, (img.rows - 1)), 0);
        x2 = std::max(std::min(x2, (img.cols - 1)), 0);
        y2 = std::max(std::min(y2, (img.rows - 1)), 0);

        pt1 = cv::Point(x1, y1);
        pt2 = cv::Point(x2, y2);
        cv::line(img, pt1, pt2, color, 2);
    }
}

void drawResults(osd_utils_img *out, float fontscale, int thickness, sample_run_joint_results *results, int offset_x, int offset_y)
{

    cv::Mat image(out->height, out->width, CV_8UC4, out->data);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        cv::Rect rect(results->mObjects[i].bbox.x * out->width + offset_x,
                      results->mObjects[i].bbox.y * out->height + offset_y,
                      results->mObjects[i].bbox.w * out->width,
                      results->mObjects[i].bbox.h * out->height);

        cv::rectangle(image, rect, cv::Scalar(255, 0, 0, 255), thickness);

        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(results->mObjects[i].objname, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);

        int x = rect.x;
        int y = rect.y - label_size.height - baseLine;
        if (y < 0)
            y = 0;
        if (x + label_size.width > image.cols)
            x = image.cols - label_size.width;

        cv::rectangle(image, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                      cv::Scalar(255, 255, 255, 255), -1);

        cv::putText(image, results->mObjects[i].objname, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale,
                    cv::Scalar(0, 0, 0, 255), thickness);

        if (results->mObjects[i].bHasPoseLmk)
        {
            draw_pose_result(image, &results->mObjects[i], SAMPLE_RUN_JOINT_POSE_LMK_SIZE, offset_x, offset_y);
        }

        if (results->mObjects[i].bHasFaceLmk)
        {
            for (size_t j = 0; j < SAMPLE_RUN_JOINT_FACE_LMK_SIZE; j++)
            {
                cv::Point p(results->mObjects[i].face_landmark[j].x * out->width + offset_x,
                            results->mObjects[i].face_landmark[j].y * out->height + offset_y);
                cv::circle(image, p, 1, cv::Scalar(255, 0, 0, 255), 2);
            }

            // draw_pose_result(image, &results->mObjects[i], SAMPLE_RUN_JOINT_POSE_LMK_SIZE, 192, 256);
        }

        if (results->mObjects[i].bHaseMask && results->mObjects[i].mYolov5Mask)
        {
            static const std::vector<std::vector<uint8_t>> COCO_COLORS = {
                {128, 56, 0, 255}, {128, 226, 255, 0}, {128, 0, 94, 255}, {128, 0, 37, 255}, {128, 0, 255, 94}, {128, 255, 226, 0}, {128, 0, 18, 255}, {128, 255, 151, 0}, {128, 170, 0, 255}, {128, 0, 255, 56}, {128, 255, 0, 75}, {128, 0, 75, 255}, {128, 0, 255, 169}, {128, 255, 0, 207}, {128, 75, 255, 0}, {128, 207, 0, 255}, {128, 37, 0, 255}, {128, 0, 207, 255}, {128, 94, 0, 255}, {128, 0, 255, 113}, {128, 255, 18, 0}, {128, 255, 0, 56}, {128, 18, 0, 255}, {128, 0, 255, 226}, {128, 170, 255, 0}, {128, 255, 0, 245}, {128, 151, 255, 0}, {128, 132, 255, 0}, {128, 75, 0, 255}, {128, 151, 0, 255}, {128, 0, 151, 255}, {128, 132, 0, 255}, {128, 0, 255, 245}, {128, 255, 132, 0}, {128, 226, 0, 255}, {128, 255, 37, 0}, {128, 207, 255, 0}, {128, 0, 255, 207}, {128, 94, 255, 0}, {128, 0, 226, 255}, {128, 56, 255, 0}, {128, 255, 94, 0}, {128, 255, 113, 0}, {128, 0, 132, 255}, {128, 255, 0, 132}, {128, 255, 170, 0}, {128, 255, 0, 188}, {128, 113, 255, 0}, {128, 245, 0, 255}, {128, 113, 0, 255}, {128, 255, 188, 0}, {128, 0, 113, 255}, {128, 255, 0, 0}, {128, 0, 56, 255}, {128, 255, 0, 113}, {128, 0, 255, 188}, {128, 255, 0, 94}, {128, 255, 0, 18}, {128, 18, 255, 0}, {128, 0, 255, 132}, {128, 0, 188, 255}, {128, 0, 245, 255}, {128, 0, 169, 255}, {128, 37, 255, 0}, {128, 255, 0, 151}, {128, 188, 0, 255}, {128, 0, 255, 37}, {128, 0, 255, 0}, {128, 255, 0, 170}, {128, 255, 0, 37}, {128, 255, 75, 0}, {128, 0, 0, 255}, {128, 255, 207, 0}, {128, 255, 0, 226}, {128, 255, 245, 0}, {128, 188, 255, 0}, {128, 0, 255, 18}, {128, 0, 255, 75}, {128, 0, 255, 151}, {128, 255, 56, 0}, {128, 245, 255, 0}};
            cv::Mat *mask = (cv::Mat *)results->mObjects[i].mYolov5Mask;
            if (!mask->empty())
            {
                cv::Mat mask_target;

                cv::resize(*mask, mask_target, cv::Size(results->mObjects[i].bbox.w * out->width, results->mObjects[i].bbox.h * out->height));

                if (results->mObjects[i].label < COCO_COLORS.size())
                {
                    image(rect).setTo(COCO_COLORS[results->mObjects[i].label], mask_target);
                }
                else
                {
                    image(rect).setTo(cv::Scalar(128, 128, 128, 128), mask_target);
                }
            }
        }
    }

    if (results->bPPHumSeg)
    {
        static cv::Mat base(SAMPLE_MAJOR_STREAM_HEIGHT,SAMPLE_MAJOR_STREAM_WIDTH,CV_8UC1);
        cv::Mat tmp(image.rows,image.cols,CV_8UC1,base.data);
        cv::Mat mask(192,192,CV_8UC1,results->mPPHumSeg.mask);
        cv::resize(mask,tmp,cv::Size(image.cols,image.rows));
        image.setTo(cv::Scalar(128, 0, 0, 128), tmp);
    }
}

int freeObjs(sample_run_joint_results *results)
{
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        if (results->mObjects[i].bHaseMask && results->mObjects[i].mYolov5Mask)
        {
            cv::Mat *mask = (cv::Mat *)results->mObjects[i].mYolov5Mask;
            results->mObjects[i].mYolov5Mask = nullptr;
        }
    }
}
