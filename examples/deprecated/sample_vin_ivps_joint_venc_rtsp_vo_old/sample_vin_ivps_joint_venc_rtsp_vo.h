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

#ifndef _SAMPLE_VIN_IVPS_JOINT_VENC_H_
#define _SAMPLE_VIN_IVPS_JOINT_VENC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "ax_venc_api.h"
#include "ax_ivps_api.h"
#include "ax_isp_api.h"

#include "common_vin.h"
#include "../utilities/sample_log.h"

#include "../common/sample_def.h"

#define SAMPLE_IVPS_GROUP_NUM (3)
#define SAMPLE_IVPS_CHN_NUM (1)
#define SAMPLE_VENC_CHN_NUM (1)
#define SAMPLE_REGION_COUNT (2)
#define SAMPLE_RECT_BOX_COUNT (16) // <=16


extern rtsp_demo_handle rDemoHandle;
extern const AX_S32 gVencChnMapping[SAMPLE_VENC_CHN_NUM];

extern IVPS_REGION_PARAM_T g_arrRgnThreadParam[SAMPLE_REGION_COUNT];

#define SAMPLE_MINOR_STREAM_WIDTH 854
#define SAMPLE_MINOR_STREAM_HEIGHT 480
#endif
