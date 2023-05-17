#pragma once
#include "ax_model_runner.hpp"
#include "utilities/object_register.hpp"

#ifdef AXERA_TARGET_CHIP_AX650
class ax_runner_ax650 : public ax_runner_base
{
protected:
    struct ax_joint_runner_ax650_handle_t *m_handle = nullptr;
    std::vector<ax_runner_tensor_t> minput_tensors;

public:
    int init(const char *model_file) override;

    void deinit() override;

    int get_algo_width() override;
    int get_algo_height() override;
    axdl_color_space_e get_color_space() override;

    int inference(axdl_image_t *pstFrame, const axdl_bbox_t *crop_resize_box) override;
};
REGISTER(RUNNER_AX650,ax_runner_ax650)
#endif