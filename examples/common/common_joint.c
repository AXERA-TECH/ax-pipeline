#include "sample_def.h"
#include "../utilities/sample_log.h"

int COMMON_JOINT_Init()
{
    if (gModels.bRunJoint == AX_TRUE)
    {
        AX_U32 s32Ret = sample_run_joint_init(gModels.MODEL_PATH, &gModels.mMajor.JointHandle, &gModels.mMajor.JointAttr);
        if (0 != s32Ret)
        {
            ALOGE("sample_run_joint_init failed,s32Ret:0x%x\n", s32Ret);
            return -1;
        }
        ALOGN("load model %s success,input resulotion width=%d height=%d !\n", gModels.MODEL_PATH, gModels.mMajor.JointAttr.algo_width, gModels.mMajor.JointAttr.algo_height);
        gModels.SAMPLE_ALGO_FORMAT = gModels.mMajor.JointAttr.algo_colorformat;
        gModels.SAMPLE_ALGO_HEIGHT = gModels.mMajor.JointAttr.algo_height;
        gModels.SAMPLE_ALGO_WIDTH = gModels.mMajor.JointAttr.algo_width;

        switch (gModels.ModelType_Main)
        {
        case MT_MLM_HUMAN_POSE_AXPPL:
        case MT_MLM_HUMAN_POSE_HRNET:
        case MT_MLM_HAND_POSE:
        case MT_MLM_FACE_RECOGNITION:
        case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
            s32Ret = sample_run_joint_init(gModels.MODEL_PATH_L2, &gModels.mMinor.JointHandle, &gModels.mMinor.JointAttr);
            if (0 != s32Ret)
            {
                ALOGE("pose:sample_run_joint_init failed,s32Ret:0x%x\n", s32Ret);
                return -1;
            }
            ALOGN("load l2 model %s success,input resulotion width=%d height=%d!\n", gModels.MODEL_PATH_L2, gModels.mMinor.JointAttr.algo_width, gModels.mMinor.JointAttr.algo_height);

            break;
        default:
            gModels.SAMPLE_IVPS_ALGO_WIDTH = gModels.mMajor.JointAttr.algo_width;
            gModels.SAMPLE_IVPS_ALGO_HEIGHT = gModels.mMajor.JointAttr.algo_height;
            break;
        }

        switch (gModels.ModelType_Main)
        {
        case MT_MLM_HUMAN_POSE_AXPPL:
        case MT_MLM_HUMAN_POSE_HRNET:
        case MT_MLM_HAND_POSE:
        case MT_MLM_FACE_RECOGNITION:
        case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
            gModels.SAMPLE_RESTORE_WIDTH = gModels.SAMPLE_IVPS_ALGO_WIDTH;
            gModels.SAMPLE_RESTORE_HEIGHT = gModels.SAMPLE_IVPS_ALGO_HEIGHT;
            break;
        default:
            gModels.SAMPLE_RESTORE_WIDTH = SAMPLE_MAJOR_STREAM_WIDTH;
            gModels.SAMPLE_RESTORE_HEIGHT = SAMPLE_MAJOR_STREAM_HEIGHT;
            break;
        }
    }
    else
    {
        ALOGN("Not specified model file\n");
    }
    return 0;
}

int COMMON_JOINT_Deinit()
{
    sample_run_joint_release(gModels.mMajor.JointHandle);
    sample_run_joint_release(gModels.mMinor.JointHandle);
}