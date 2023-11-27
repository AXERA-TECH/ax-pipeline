#include "ax_model_custom.hpp"
#include "../../utilities/sample_log.h"

int ax_model_custom::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    /*
     *  用户可以在这里进行任意自定义后处理代码编写
     */

    // 这是用户自定义模型的输出节点的数量
    int nOutputSize = m_runner->get_num_outputs();
    // 这是用户自定义模型的输出节点的信息对应的结构体指针，详情请浏览 ax_runner_tensor_t 结构体信息
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    // 后处理代码

    // 用户可以根据后处理结果，在此处对 axdl_results_t *results 进行填充
    results->nObjSize = 1;
    results->mObjects[0].label = 0;
    results->mObjects[0].prob = 1;
    results->mObjects[0].bbox = {50, 50, 100, 100};
    return 0;
}

void ax_model_custom::draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    /*
     * 用户可以在这里，对后处理结果进行绘制，此函数可以使用opencv，绘制到 cv::Mat &image 中。
     * 注意：
     * 1、cv::Mat &image 是一个BGRA的四通道图像
     * 2、请不要对 cv::Mat &image 进行除了绘制图案之外的其他修改，如 resize、cvtColor 等
     */
    draw_bbox(image, results, fontscale, thickness, offset_x, offset_y);
}

void ax_model_custom::draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness)
{
    /*
     * 用户可以在这里吗，对后处理结果进行绘制。此处使用的是原生的api进行绘制，效率会比opencv快，但是具有一定的限制性
     * 注意：
     * 1、使用 m_drawers[chn] 的接口进行绘制，如果 m_drawers[chn] 不能满足绘制需求，则无法绘制
     * 2、详情可以参考 examples/libaxdl/include/ax_osd_drawer.hpp 定义的结构体
     */
    draw_bbox(chn, results, fontscale, thickness);
}