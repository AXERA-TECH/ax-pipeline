#include "ax_model_runner.hpp"
#include "utilities/object_register.hpp"

class ax_runner_ax620 : public ax_runner_base
{
protected:
    struct ax_joint_runner_ax620_handle_t *m_handle = nullptr;

public:
    int init(const char *model_file) override;

    void deinit() override;

    int get_algo_width() override;
    int get_algo_height() override;
    axdl_color_space_e get_color_space() override;

    int inference(axdl_image_t *pstFrame, const axdl_bbox_t *crop_resize_box) override;
};
REGISTER(RUNNER_AX620,ax_runner_ax620)