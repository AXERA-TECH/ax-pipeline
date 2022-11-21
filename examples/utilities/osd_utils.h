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

#ifndef _OSD_UTILS_H_
#define _OSD_UTILS_H_
typedef struct _osd_utils_img
{
    unsigned char *data;
    int width, height, channel;
} osd_utils_img;

#ifdef __cplusplus
extern "C"
{
#endif
    /// @brief 创建一张四通道透明背景图片，用作 osd 绘制字符串
    /// @param charlen 图片最大支持的字符串长度
    /// @param fontscale 图片中字符串的大小，详情参考opencv中putText的 fontscale 参数
    /// @param out 创建的图片
    void genImg(int charlen, float fontscale, int thickness, osd_utils_img *out);
    /// @brief 在图片中绘制透明背景的字符串，输出的图片是从预先创建的图片上截取的内存创建的，此操作目的是为了减少内存申请，请不要重复释放
    /// @param text 需要绘制的字符串
    /// @param fontscale 图片中字符串的大小，详情参考opencv中putText的 fontscale 参数
    /// @param base 预先创建的四通道透明图片
    /// @param out 从预先创建的四通道透明图片中创建的图片
    /// @return
    int putText(char *text, float fontscale, int thickness, osd_utils_img *base, osd_utils_img *out);
    void releaseImg(osd_utils_img *img);
    void drawResults(osd_utils_img *out, float fontscale, int thickness, sample_run_joint_results *results, int offset_x, int offset_y);
    int freeObjs(sample_run_joint_results *results);
#ifdef __cplusplus
}
#endif
#endif