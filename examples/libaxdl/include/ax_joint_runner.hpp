#pragma once
#include "sample_run_joint.h"

typedef struct
{
    float fX, fY, fW, fH;
} ax_joint_runner_box;

class ax_joint_runner
{
protected:
    void *m_handle = nullptr;

public:
    sample_run_joint_attr m_attr = {0};

    ax_joint_runner(/* args */) {}
    ~ax_joint_runner() {}
    int init(const char *model_file);

    void deinit();

    int inference(const void *pstFrame, const ax_joint_runner_box *crop_resize_box);
};
