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

#include "ax_interpreter_external_api.h"
#include "ax_sys_api.h"
#include "joint.h"
#include "joint_adv.h"
#include "npu_cv_kit/ax_npu_imgproc.h"
#define AXERA_TARGET_CHIP_AX620

#include "utilities/file.hpp"
#include "middleware/io.hpp"
#include "sample_run_joint.h"
#include "../utilities/sample_log.h"
#include <vector>

void sample_run_joint_post_process(AX_U32 nOutputSize, AX_JOINT_IOMETA_T *pOutputsInfo, AX_JOINT_IO_BUFFER_T *pOutputs, sample_run_joint_results *pResults,
                                   int SAMPLE_ALGO_WIDTH, int SAMPLE_ALGO_HEIGHT, int SAMPLE_MAJOR_STREAM_WIDTH, int SAMPLE_MAJOR_STREAM_HEIGHT);

typedef struct
{
    AX_JOINT_HANDLE joint_handle;
    AX_JOINT_SDK_ATTR_T joint_attr;

    // AX_JOINT_IO_INFO_T* io_info = nullptr;

    AX_JOINT_EXECUTION_CONTEXT joint_ctx;
    AX_JOINT_EXECUTION_CONTEXT_SETTING_T joint_ctx_settings;

    AX_JOINT_IO_T joint_io_arr;
    AX_JOINT_IO_SETTING_T joint_io_setting;

    AX_NPU_CV_Image pDstNV12;

    int SAMPLE_ALGO_WIDTH = 0;
    int SAMPLE_ALGO_HEIGHT = 0;
} handle_t;

AX_S32 alloc_joint_buffer(const AX_JOINT_IOMETA_T *pMeta, AX_JOINT_IO_BUFFER_T *pBuf)
{
    // AX_JOINT_IOMETA_T meta = *pMeta;
    auto ret = AX_JOINT_AllocBuffer(pMeta, pBuf, AX_JOINT_ABST_DEFAULT);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret)
    {
        fprintf(stderr, "[ERR]: Cannot allocate memory.\n");
        return -1;
    }
    return AX_ERR_NPU_JOINT_SUCCESS;
}

AX_S32 prepare_io(AX_NPU_CV_Image *pDstNV12, AX_JOINT_IO_T &io, const AX_JOINT_IO_INFO_T *io_info, const uint32_t &batch)
{
    memset(&io, 0, sizeof(io));

    io.nInputSize = io_info->nInputSize;
    if (1 != io.nInputSize)
    {
        fprintf(stderr, "[ERR]: Only single input was accepted(got %u).\n", io.nInputSize);
        return -1;
    }
    io.pInputs = new AX_JOINT_IO_BUFFER_T[io.nInputSize];

    // fill input
    {
        const AX_JOINT_IOMETA_T *pMeta = io_info->pInputs;
        AX_JOINT_IO_BUFFER_T *pBuf = io.pInputs;

        if (pMeta->nShapeSize <= 0)
        {
            fprintf(stderr, "[ERR]: Dimension(%u) of shape is not allowed.\n", (uint32_t)pMeta->nShapeSize);
            return -1;
        }

        auto actual_data_size = pMeta->nSize / pMeta->pShape[0] * batch;
        if (pDstNV12->nSize != actual_data_size)
        {
            fprintf(stderr,
                    "[ERR]: The buffer size is not equal to model input(%s) size(%u vs %u).\n",
                    io_info->pInputs[0].pName,
                    (uint32_t)pDstNV12->nSize,
                    actual_data_size);
            return -1;
        }
        pBuf->phyAddr = (AX_ADDR)pDstNV12->pPhy;
        pBuf->pVirAddr = (AX_VOID *)pDstNV12->pVir;
        pBuf->nSize = (AX_U32)pDstNV12->nSize;
    }

    // deal with output
    {
        io.nOutputSize = io_info->nOutputSize;
        io.pOutputs = new AX_JOINT_IO_BUFFER_T[io.nOutputSize];
        for (size_t i = 0; i < io.nOutputSize; ++i)
        {
            const AX_JOINT_IOMETA_T *pMeta = io_info->pOutputs + i;
            AX_JOINT_IO_BUFFER_T *pBuf = io.pOutputs + i;
            alloc_joint_buffer(pMeta, pBuf);
        }
    }
    return AX_ERR_NPU_JOINT_SUCCESS;
}

int npu_crop_resize(const AX_NPU_CV_Image *input_image, AX_NPU_CV_Image *output_image, AX_NPU_CV_Box *box,
                    AX_NPU_SDK_EX_MODEL_TYPE_T model_type, AX_NPU_CV_ImageResizeAlignParam horizontal,
                    AX_NPU_CV_ImageResizeAlignParam vertical)
{
    AX_NPU_CV_Color color;
    color.nYUVColorValue[0] = 128;
    color.nYUVColorValue[1] = 128;
    AX_NPU_SDK_EX_MODEL_TYPE_T virtual_npu_mode_type = model_type;

    AX_NPU_CV_Box *ppBox[1];
    ppBox[0] = box;

    int ret = AX_NPU_CV_CropResizeImage(virtual_npu_mode_type, input_image, 1, &output_image, ppBox, horizontal, vertical, color);

    if (ret != AX_NPU_DEV_STATUS_SUCCESS)
    {
        ALOGE("AX_NPU_CV_CropResizeImage err code: %X", ret);
        return ret;
    }

    return 0;
}

int sample_run_joint_init(char *model_file, void **yhandle, int *algo_width, int *algo_height)
{
    if (!model_file)
    {
        ALOGE("invalid param:model_file is null");
        return -1;
    }

    handle_t *handle = new handle_t;

    // 1. create a runtime handle and load the model
    // AX_JOINT_HANDLE joint_handle;
    memset(&handle->joint_handle, 0, sizeof(handle->joint_handle));

    memset(&handle->joint_attr, 0, sizeof(handle->joint_attr));

    // 1.1 read model file to buffer
    std::vector<char> model_buffer;
    if (!utilities::read_file(model_file, model_buffer))
    {
        fprintf(stderr, "Read Run-Joint model(%s) file failed.\n", model_file);
        return -1;
    }

    auto ret = middleware::parse_npu_mode_from_joint(model_buffer.data(), model_buffer.size(), &handle->joint_attr.eNpuMode);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret)
    {
        fprintf(stderr, "Load Run-Joint model(%s) failed.\n", model_file);
        return -1;
    }

    // 1.3 init model
    ret = AX_JOINT_Adv_Init(&handle->joint_attr);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret)
    {
        fprintf(stderr, "Init Run-Joint model(%s) failed.\n", model_file);
        return -1;
    }

    auto deinit_joint = [&]()
    {
        AX_JOINT_DestroyHandle(handle->joint_handle);
        AX_JOINT_Adv_Deinit();
        return -1;
    };

    // 1.4 the real init processing

    ret = AX_JOINT_CreateHandle(&handle->joint_handle, model_buffer.data(), model_buffer.size());
    if (AX_ERR_NPU_JOINT_SUCCESS != ret)
    {
        fprintf(stderr, "Create Run-Joint handler from file(%s) failed.\n", model_file);
        return deinit_joint();
    }

    // 1.5 get the version of toolkit (optional)
    const AX_CHAR *version = AX_JOINT_GetModelToolsVersion(handle->joint_handle);
    fprintf(stdout, "Tools version: %s\n", version);

    // 1.6 drop the model buffer
    std::vector<char>().swap(model_buffer);

    // 1.7 create context
    memset(&handle->joint_ctx, 0, sizeof(handle->joint_ctx));
    memset(&handle->joint_ctx_settings, 0, sizeof(handle->joint_ctx_settings));
    ret = AX_JOINT_CreateExecutionContextV2(handle->joint_handle, &handle->joint_ctx, &handle->joint_ctx_settings);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret)
    {
        fprintf(stderr, "Create Run-Joint context failed.\n");
        return deinit_joint();
    }

    memset(&handle->joint_io_arr, 0, sizeof(handle->joint_io_arr));
    memset(&handle->joint_io_setting, 0, sizeof(handle->joint_io_setting));
    memset(&handle->pDstNV12, 0, sizeof(handle->pDstNV12));

    auto io_info = AX_JOINT_GetIOInfo(handle->joint_handle);
    handle->SAMPLE_ALGO_HEIGHT = io_info->pInputs->pShape[1] / 1.5;
    handle->SAMPLE_ALGO_WIDTH = io_info->pInputs->pShape[2];
    if (algo_width)
    {
        *algo_width = handle->SAMPLE_ALGO_WIDTH;
    }
    if (algo_height)
    {
        *algo_height = handle->SAMPLE_ALGO_HEIGHT;
    }

    handle->pDstNV12.nWidth = handle->SAMPLE_ALGO_WIDTH;
    handle->pDstNV12.nHeight = handle->SAMPLE_ALGO_HEIGHT;
    handle->pDstNV12.tStride.nW = handle->SAMPLE_ALGO_WIDTH;
    handle->pDstNV12.eDtype = AX_NPU_CV_FDT_NV12;
    handle->pDstNV12.nSize = handle->pDstNV12.nWidth * handle->pDstNV12.nHeight * 1.5;
    ret = AX_SYS_MemAlloc((AX_U64 *)&handle->pDstNV12.pPhy, (void **)&handle->pDstNV12.pVir, handle->pDstNV12.nSize, 0x100, (AX_S8 *)"SAMPLE-CV");
    if (ret != AX_ERR_NPU_JOINT_SUCCESS)
    {
        ALOGE("error alloc image sys mem %x", ret);
        return -1;
    }

    ret = prepare_io(&handle->pDstNV12, handle->joint_io_arr, io_info, 1);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret)
    {
        fprintf(stderr, "Fill input failed.\n");
        AX_JOINT_DestroyExecutionContext(handle->joint_ctx);
        return deinit_joint();
    }
    handle->joint_io_arr.pIoSetting = &handle->joint_io_setting;
    *yhandle = handle;
    return 0;
}

int sample_run_joint_release(void *yhandle)
{
    handle_t *handle = (handle_t *)yhandle;
    if (handle)
    {
        auto DestroyJoint = [&]()
        {
            if (handle->joint_io_arr.pInputs)
            {
                delete[] handle->joint_io_arr.pInputs;
            }

            if (handle->joint_io_arr.pOutputs)
            {
                for (size_t i = 0; i < handle->joint_io_arr.nOutputSize; ++i)
                {
                    AX_JOINT_IO_BUFFER_T *pBuf = handle->joint_io_arr.pOutputs + i;
                    AX_JOINT_FreeBuffer(pBuf);
                }

                delete[] handle->joint_io_arr.pOutputs;
            }

            AX_JOINT_DestroyExecutionContext(handle->joint_ctx);
            AX_JOINT_DestroyHandle(handle->joint_handle);
            AX_JOINT_Adv_Deinit();
        };
        DestroyJoint();
        AX_SYS_MemFree((AX_U64)handle->pDstNV12.pPhy, (void *)handle->pDstNV12.pVir);

        delete handle;
    }
    return 0;
}

int sample_run_joint_inference(void *yhandle, const void *_pstFrame, sample_run_joint_results *pResults, int src_width, int src_height)
{
    handle_t *handle = (handle_t *)yhandle;

    if (!handle)
    {
        ALOGE("invalid param:yhandle is null");
        return -1;
    }

    if (!pResults)
    {
        ALOGE("invalid param:pResults is null");
        return -1;
    }
    AX_NPU_CV_Image *pstFrame = (AX_NPU_CV_Image *)_pstFrame;
    // check eImgType
    if (AX_NPU_CV_FDT_NV12 != pstFrame->eDtype)
    {
        ALOGE("Only support NV12");
        return -1;
    }
    AX_NPU_SDK_EX_MODEL_TYPE_T ModelType;
    AX_JOINT_GetVNPUMode(handle->joint_handle, &ModelType);
    npu_crop_resize(pstFrame, &handle->pDstNV12, NULL, ModelType, AX_NPU_CV_IMAGE_HORIZONTAL_CENTER, AX_NPU_CV_IMAGE_VERTICAL_CENTER);

    auto ret = AX_JOINT_RunSync(handle->joint_handle, handle->joint_ctx, &handle->joint_io_arr);

    // 5. post process
    auto io_info = AX_JOINT_GetIOInfo(handle->joint_handle);
    sample_run_joint_post_process(io_info->nOutputSize, io_info->pOutputs, handle->joint_io_arr.pOutputs, pResults,
                                  handle->SAMPLE_ALGO_WIDTH, handle->SAMPLE_ALGO_HEIGHT, src_width, src_height);
    return 0;
}
