#ifndef _COMMON_JOINT_H_
#define _COMMON_JOINT_H_

#include "../sample_run_joint/sample_run_joint_post_process.h"
#if __cplusplus
extern "C"
{
#endif
    int COMMON_JOINT_Init(sample_run_joint_models *pModels, int default_image_width, int default_image_height);
    int COMMON_JOINT_Deinit(sample_run_joint_models *pModels);
#if __cplusplus
}
#endif
#endif