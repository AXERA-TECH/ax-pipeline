// #include "sample_def.h"
#include "common_joint.h"
#include "../utilities/sample_log.h"

int COMMON_JOINT_Init(sample_run_joint_models *pModels, int default_image_width, int default_image_height)
{
    if (pModels->bRunJoint == AX_TRUE)
    {
        int s32Ret = sample_run_joint_init(pModels->MODEL_PATH, &pModels->mMajor.JointHandle, &pModels->mMajor.JointAttr);
        if (0 != s32Ret)
        {
            ALOGE("sample_run_joint_init failed,s32Ret:0x%x\n", s32Ret);
            return -1;
        }
        ALOGN("load model %s success,input resulotion width=%d height=%d !\n", pModels->MODEL_PATH, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height);
        pModels->SAMPLE_ALGO_FORMAT = pModels->mMajor.JointAttr.algo_colorformat;
        pModels->SAMPLE_ALGO_HEIGHT = pModels->mMajor.JointAttr.algo_height;
        pModels->SAMPLE_ALGO_WIDTH = pModels->mMajor.JointAttr.algo_width;

        switch (pModels->ModelType_Main)
        {
        case MT_MLM_HUMAN_POSE_AXPPL:
        case MT_MLM_HUMAN_POSE_HRNET:
        case MT_MLM_ANIMAL_POSE_HRNET:
        case MT_MLM_HAND_POSE:
        case MT_MLM_FACE_RECOGNITION:
        case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
            s32Ret = sample_run_joint_init(pModels->MODEL_PATH_L2, &pModels->mMinor.JointHandle, &pModels->mMinor.JointAttr);
            if (0 != s32Ret)
            {
                ALOGE("pose:sample_run_joint_init failed,s32Ret:0x%x\n", s32Ret);
                return -1;
            }
            ALOGN("load l2 model %s success,input resulotion width=%d height=%d!\n", pModels->MODEL_PATH_L2, pModels->mMinor.JointAttr.algo_width, pModels->mMinor.JointAttr.algo_height);

            break;
        default:
            pModels->SAMPLE_IVPS_ALGO_WIDTH = pModels->mMajor.JointAttr.algo_width;
            pModels->SAMPLE_IVPS_ALGO_HEIGHT = pModels->mMajor.JointAttr.algo_height;
            break;
        }

        switch (pModels->ModelType_Main)
        {
        case MT_MLM_HUMAN_POSE_AXPPL:
        case MT_MLM_HUMAN_POSE_HRNET:
        case MT_MLM_ANIMAL_POSE_HRNET:
        case MT_MLM_HAND_POSE:
        case MT_MLM_FACE_RECOGNITION:
        case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
            pModels->SAMPLE_RESTORE_WIDTH = pModels->SAMPLE_IVPS_ALGO_WIDTH;
            pModels->SAMPLE_RESTORE_HEIGHT = pModels->SAMPLE_IVPS_ALGO_HEIGHT;
            break;
        default:
            pModels->SAMPLE_RESTORE_WIDTH = default_image_width;
            pModels->SAMPLE_RESTORE_HEIGHT = default_image_height;
            break;
        }
    }
    else
    {
        ALOGN("Not specified model file\n");
    }
    return 0;
}

int COMMON_JOINT_Deinit(sample_run_joint_models *pModels)
{
    if (pModels->bRunJoint == AX_TRUE)
    {
        sample_run_joint_release(pModels->mMajor.JointHandle);
        sample_run_joint_release(pModels->mMinor.JointHandle);
    }
    return 0;
}