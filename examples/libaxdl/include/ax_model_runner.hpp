#pragma once
#include "vector"
#include "string"

#include "ax_common_api.h"

typedef struct
{
    std::string sName;
    unsigned int nIdx;
    std::vector<unsigned int> vShape;
    int nSize;
    unsigned long phyAddr;
    void *pVirAddr;
} ax_runner_tensor_t;

class ax_runner_base
{
protected:
    std::vector<ax_runner_tensor_t> mtensors;

public:
    virtual int init(const char *model_file) = 0;

    virtual void deinit() = 0;

    int get_num_outputs() { return mtensors.size(); };

    const ax_runner_tensor_t &get_output(int idx) { return mtensors[idx]; }
    const ax_runner_tensor_t *get_outputs_ptr() { return mtensors.data(); }

    virtual int get_algo_width() = 0;
    virtual int get_algo_height() = 0;
    virtual axdl_color_space_e get_color_space() = 0;

    virtual int inference(axdl_image_t *pstFrame, const axdl_bbox_t *crop_resize_box) = 0;
};
