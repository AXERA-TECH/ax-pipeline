#include "ax_model_runner_axcl.hpp"
#include "logger.hpp"

#include <string.h>
#include <fstream>
#include <memory>
#include <fcntl.h>
// #include <axcl.h>
#include "axcl_manager.h"

// #include <opencv2/opencv.hpp>

typedef enum
{
    AX_ENGINE_ABST_DEFAULT = 0,
    AX_ENGINE_ABST_CACHED = 1,
} AX_ENGINE_ALLOC_BUFFER_STRATEGY_T;

typedef std::pair<AX_ENGINE_ALLOC_BUFFER_STRATEGY_T, AX_ENGINE_ALLOC_BUFFER_STRATEGY_T> INPUT_OUTPUT_ALLOC_STRATEGY;

void print_io_info(std::vector<ax_runner_tensor_t> &input, std::vector<ax_runner_tensor_t> &output)
{
    printf("\ninput size: %ld\n", input.size());
    for (size_t i = 0; i < input.size(); ++i)
    {
        // print shape info,like [batchsize x channel x height x width]
        auto &info = input[i];
        printf("    name: \e[1;32m%8s", info.sName.c_str());

        std::string dt = "unknown";

        printf(" \e[1;31m[%s] ", dt.c_str());

        std::string ct = "unknown";

        printf("\e[1;31m[%s]", ct.c_str());

        printf(" \n        \e[1;31m");

        for (size_t s = 0; s < info.vShape.size(); s++)
        {
            printf("%d", info.vShape[s]);
            if (s != info.vShape.size() - 1)
            {
                printf(" x ");
            }
        }
        printf("\e[0m\n\n");
    }

    printf("\noutput size: %ld\n", output.size());
    for (size_t i = 0; i < output.size(); ++i)
    {
        // print shape info,like [batchsize x channel x height x width]
        auto &info = output[i];
        printf("    name: \e[1;32m%8s \e[0m\n        \e[1;31m", info.sName.c_str());
        for (size_t s = 0; s < info.vShape.size(); s++)
        {
            printf("%d", info.vShape[s]);
            if (s != info.vShape.size() - 1)
            {
                printf(" x ");
            }
        }
        printf("\e[0m\n\n");
    }
}

typedef struct
{
    int nIndex;
    int nSize;
    void *pBuf;
    void *pVirAddr;

    std::string Name;

    axclrtEngineIODims dims;
} AXCL_IO_BUF_T;

typedef struct
{
    uint32_t nInputSize;
    uint32_t nOutputSize;
    AXCL_IO_BUF_T *pInputs;
    AXCL_IO_BUF_T *pOutputs;
} AXCL_IO_DATA_T;

static void free_io_index(AXCL_IO_BUF_T *pBuf, size_t index, int _devid)
{
    for (size_t i = 0; i < index; ++i)
    {
        axcl_Free(pBuf[i].pBuf, _devid);
    }
}

static void free_io(AXCL_IO_DATA_T *io_data, int _devid)
{
    for (size_t j = 0; j < io_data->nInputSize; ++j)
    {
        axcl_Free(io_data->pInputs[j].pBuf, _devid);
    }
    for (size_t j = 0; j < io_data->nOutputSize; ++j)
    {
        axcl_Free(io_data->pOutputs[j].pBuf, _devid);
    }
    delete[] io_data->pInputs;
    delete[] io_data->pOutputs;
}

static inline int prepare_io(int grpid, axclrtEngineIOInfo io_info, axclrtEngineIO io, AXCL_IO_DATA_T *io_data, INPUT_OUTPUT_ALLOC_STRATEGY strategy, int devid)
{
    memset(io_data, 0, sizeof(AXCL_IO_DATA_T));

    auto inputNum = axcl_EngineGetNumInputs(io_info, devid);
    auto outputNum = axcl_EngineGetNumOutputs(io_info, devid);
    io_data->nInputSize = inputNum;
    io_data->nOutputSize = outputNum;
    io_data->pInputs = new AXCL_IO_BUF_T[inputNum];
    io_data->pOutputs = new AXCL_IO_BUF_T[outputNum];

    // 1. alloc inputs
    for (uint32_t i = 0; i < inputNum; i++)
    {
        auto bufSize = axcl_EngineGetInputSizeByIndex(io_info, grpid, i, devid);
        void *devPtr = nullptr;
        axclError ret = 0;
        if (AX_ENGINE_ABST_DEFAULT == strategy.first)
        {
            ret = axcl_Malloc(&devPtr, bufSize, axclrtMemMallocPolicy::AXCL_MEM_MALLOC_HUGE_FIRST, devid);
        }
        else
        {
            ret = axcl_MallocCached(&devPtr, bufSize, axclrtMemMallocPolicy::AXCL_MEM_MALLOC_HUGE_FIRST, devid);
        }

        if (ret != 0)
        {
            free_io_index(io_data->pInputs, i, devid);
            ALOGE("Malloc input(index: %d, size: %ld) failed! ret=0x%x", i, bufSize, ret);
            return -1;
        }
        std::vector<char> tmp(bufSize, 0);
        axcl_Memcpy(devPtr, tmp.data(), bufSize, axclrtMemcpyKind::AXCL_MEMCPY_HOST_TO_DEVICE, devid);
        // axclrtMemset(devPtr, 0, bufSize);

        axclrtEngineIODims dims;
        ret = axcl_EngineGetInputDims(io_info, grpid, i, &dims, devid);
        if (ret != 0)
        {
            free_io_index(io_data->pInputs, i, devid);
            ALOGE("Get input dims(index: %d) failed! ret=0x%x", i, ret);
            return -1;
        }

        io_data->pInputs[i].nIndex = i;
        io_data->pInputs[i].nSize = bufSize;
        io_data->pInputs[i].pBuf = devPtr;
        io_data->pInputs[i].dims = dims;
        io_data->pInputs[i].Name = axcl_EngineGetInputNameByIndex(io_info, i, devid);
        io_data->pInputs[i].pVirAddr = malloc(bufSize);
        memset(io_data->pInputs[i].pVirAddr, 0, bufSize);
        ret = axcl_EngineSetInputBufferByIndex(io, i, devPtr, bufSize, devid);
        if (ret != 0)
        {
            free_io_index(io_data->pInputs, i, devid);
            ALOGE("Set input buffer(index: %d, size: %lu) failed! ret=0x%x", i, bufSize, ret);
            return -1;
        }
    }

    // 2. alloc outputs
    for (uint32_t i = 0; i < outputNum; i++)
    {
        auto bufSize = axcl_EngineGetOutputSizeByIndex(io_info, grpid, i, devid);
        void *devPtr = NULL;
        axclError ret = 0;
        if (AX_ENGINE_ABST_DEFAULT == strategy.first)
        {
            ret = axcl_Malloc(&devPtr, bufSize, axclrtMemMallocPolicy::AXCL_MEM_MALLOC_HUGE_FIRST, devid);
        }
        else
        {
            ret = axcl_MallocCached(&devPtr, bufSize, axclrtMemMallocPolicy::AXCL_MEM_MALLOC_HUGE_FIRST, devid);
        }

        if (ret != 0)
        {
            free_io_index(io_data->pOutputs, i, devid);
            ALOGE("Malloc output(index: %d, size: %ld) failed! ret=0x%x", i, bufSize, ret);
            return -1;
        }
        std::vector<char> tmp(bufSize, 0);
        axcl_Memcpy(devPtr, tmp.data(), bufSize, axclrtMemcpyKind::AXCL_MEMCPY_HOST_TO_DEVICE, devid);
        axclrtEngineIODims dims;
        ret = axcl_EngineGetOutputDims(io_info, grpid, i, &dims, devid);
        if (ret != 0)
        {
            free_io_index(io_data->pOutputs, i, devid);
            ALOGE("Get output dims(index: %d) failed! ret=0x%x", i, ret);
            return -1;
        }

        io_data->pOutputs[i].nIndex = i;
        io_data->pOutputs[i].nSize = bufSize;
        io_data->pOutputs[i].pBuf = devPtr;
        io_data->pOutputs[i].dims = dims;
        io_data->pOutputs[i].Name = axcl_EngineGetOutputNameByIndex(io_info, i, devid);
        io_data->pOutputs[i].pVirAddr = malloc(bufSize);
        memset(io_data->pOutputs[i].pVirAddr, 0, bufSize);
        ret = axcl_EngineSetOutputBufferByIndex(io, i, devPtr, bufSize, devid);
        if (ret != 0)
        {
            free_io_index(io_data->pOutputs, i, devid);
            ALOGE("Set output buffer(index: %d, size: %lu) failed! ret=0x%x", i, bufSize, ret);
            return -1;
        }
    }

    return 0;
}

struct ax_joint_runner_axcl_handle_t
{
    uint64_t handle = 0;
    uint64_t context = 0;
    axclrtEngineIOInfo io_info = 0;
    std::vector<axclrtEngineIO> ios;
    std::vector<AXCL_IO_DATA_T> io_datas;

    // int algo_width, algo_height;
    // int algo_colorformat;
};

int ax_runner_axcl::sub_init()
{
    // 4. create context
    int ret = axcl_EngineCreateContext(m_handle->handle, &m_handle->context, _devid);
    if (0 != ret)
    {
        ALOGE("axclrtEngineCreateContext failed.");
        return ret;
    }
    // fprintf(stdout, "axclrtEngineCreateContextt is done. \n");

    // 5. set io

    ret = axcl_EngineGetIOInfo(m_handle->handle, &m_handle->io_info, _devid);
    if (0 != ret)
    {
        ALOGE("axclrtEngineGetIOInfo failed.");
        return ret;
    }
    // fprintf(stdout, "axclrtEngineGetIOInfo is done. \n");

    ret = axcl_EngineGetShapeGroupsCount(m_handle->io_info, &group_count, _devid);
    if (ret != 0)
    {
        axcl_EngineUnload(m_handle->handle, _devid);
        return ret;
    }

    // 6. alloc io

    m_handle->ios.resize(group_count);
    m_handle->io_datas.resize(group_count);
    mgroup_input_tensors.resize(group_count);
    mgroup_output_tensors.resize(group_count);

    memset(&m_handle->io_datas[0], 0, sizeof(AXCL_IO_DATA_T) * group_count);

    auto malloc_strategy = std::make_pair(AX_ENGINE_ABST_DEFAULT, AX_ENGINE_ABST_DEFAULT);

    for (int grpid = 0; grpid < group_count; grpid++)
    {
        ret = axcl_EngineCreateIO(m_handle->io_info, &m_handle->ios[grpid], _devid);
        if (ret != 0)
        {
            axcl_EngineUnload(m_handle->handle, _devid);
            ALOGE("Create io failed. ret=0x%x", ret);
            return -1;
        }

        ret = prepare_io(grpid, m_handle->io_info, m_handle->ios[grpid], &m_handle->io_datas[grpid], malloc_strategy, _devid);
        if (ret != 0)
        {
            free_io(&m_handle->io_datas[grpid], _devid);
            axcl_EngineDestroyIO(m_handle->ios[grpid], _devid);
            axcl_EngineUnload(m_handle->handle, _devid);

            ALOGE("prepare_io failed.");
            return ret;
        }
    }

    for (int grpid = 0; grpid < group_count; grpid++)
    {
        // auto &io_info = m_handle->io_info[grpid];
        auto &io_data = m_handle->io_datas[grpid];
        for (uint32_t i = 0; i < io_data.nOutputSize; i++)
        {
            ax_runner_tensor_t tensor;
            tensor.nIdx = i;
            tensor.sName = std::string(io_data.pOutputs[i].Name);
            tensor.nSize = io_data.pOutputs[i].nSize;
            for (int32_t j = 0; j < io_data.pOutputs[i].dims.dimCount; j++)
            {
                tensor.vShape.push_back(io_data.pOutputs[i].dims.dims[j]);
            }
            // tensor.eColorSpace = ax_color_space_unknown;
            tensor.phyAddr = (unsigned long long)io_data.pOutputs[i].pBuf;
            tensor.pVirAddr = io_data.pOutputs[i].pVirAddr;
            mgroup_output_tensors[grpid].push_back(tensor);
        }

        for (size_t i = 0; i < io_data.nInputSize; i++)
        {
            ax_runner_tensor_t tensor;
            tensor.nIdx = i;
            tensor.sName = std::string(io_data.pInputs[i].Name);
            tensor.nSize = io_data.pInputs[i].nSize;
            for (int32_t j = 0; j < io_data.pInputs[i].dims.dimCount; j++)
            {
                tensor.vShape.push_back(io_data.pInputs[i].dims.dims[j]);
            }
            // tensor.eColorSpace = ax_color_space_unknown;
            tensor.phyAddr = (unsigned long long)io_data.pInputs[i].pBuf;
            tensor.pVirAddr = io_data.pInputs[i].pVirAddr;
            mgroup_input_tensors[grpid].push_back(tensor);
        }
        // print_io_info(mgroup_input_tensors[grpid], mgroup_output_tensors[grpid]);
    }

    moutput_tensors = mgroup_output_tensors[0];
    minput_tensors = mgroup_input_tensors[0];

    // for (int grpid = 0; grpid < group_count; grpid++)
    // {
    //     printf("\ngrpid: %d\n", grpid);
    //     print_io_info(mgroup_input_tensors[grpid], mgroup_output_tensors[grpid]);
    //     printf("==================================================\n\n");
    // }

    return ret;
}

int ax_runner_axcl::init(const void *model_data, unsigned int model_size, int devid)
{
    if (!m_handle)
    {
        m_handle = new ax_joint_runner_axcl_handle_t;
    }
    memset((void *)m_handle, 0, sizeof(ax_joint_runner_axcl_handle_t));

    _devid = devid;

    // 3. create handle
    void *devMem = nullptr;
    axcl_Malloc(&devMem, model_size, AXCL_MEM_MALLOC_NORMAL_ONLY, _devid);

    // 4. copy model to device
    axcl_Memcpy(devMem, model_data, model_size, AXCL_MEMCPY_HOST_TO_DEVICE, _devid);

    int ret = axcl_EngineLoadFromMem(devMem, model_size, &m_handle->handle, _devid);
    if (0 != ret)
    {
        ALOGE("AX_ENGINE_CreateHandle");
        return ret;
    }
    axcl_Free(devMem, _devid);

    return sub_init();
}

void ax_runner_axcl::deinit()
{
    if (m_handle && m_handle->handle)
    {
        for (int grpid = 0; grpid < group_count; grpid++)
        {
            free_io(&m_handle->io_datas[grpid], _devid);
            axcl_EngineDestroyIO(m_handle->ios[grpid], _devid);
        }

        axcl_EngineUnload(m_handle->handle, _devid);
        m_handle->handle = 0;
    }

    if (m_handle)
    {
        delete m_handle;
        m_handle = nullptr;
    }

    minput_tensors.clear();
    moutput_tensors.clear();

    map_input_tensors.clear();
    map_output_tensors.clear();

    mgroup_input_tensors.clear();
    mgroup_output_tensors.clear();

    map_group_input_tensors.clear();
    map_group_output_tensors.clear();
}

int ax_runner_axcl::set_affinity(int id)
{
    return axcl_EngineSetAffinity(m_handle->handle, id, _devid);
}

int ax_runner_axcl::sync_input(int idx)
{
    auto &input = get_input(idx);
    return axcl_Memcpy((void *)input.phyAddr, input.pVirAddr, input.nSize, AXCL_MEMCPY_HOST_TO_DEVICE, _devid);
}

int ax_runner_axcl::sync_input(std::string name)
{
    auto &input = get_input(name);
    return axcl_Memcpy((void *)input.phyAddr, input.pVirAddr, input.nSize, AXCL_MEMCPY_HOST_TO_DEVICE, _devid);
}

int ax_runner_axcl::sync_output(int idx)
{
    auto &output = get_output(idx);
    return axcl_Memcpy(output.pVirAddr, (void *)output.phyAddr, output.nSize, AXCL_MEMCPY_DEVICE_TO_HOST, _devid);
}

int ax_runner_axcl::sync_output(std::string name)
{
    auto &output = get_output(name);
    return axcl_Memcpy(output.pVirAddr, (void *)output.phyAddr, output.nSize, AXCL_MEMCPY_DEVICE_TO_HOST, _devid);
}

int ax_runner_axcl::sync_input(int grpid, int idx)
{
    auto &input = get_input(grpid, idx);
    return axcl_Memcpy((void *)input.phyAddr, input.pVirAddr, input.nSize, AXCL_MEMCPY_HOST_TO_DEVICE, _devid);
}

int ax_runner_axcl::sync_input(int grpid, std::string name)
{
    auto &input = get_input(grpid, name);
    return axcl_Memcpy((void *)input.phyAddr, input.pVirAddr, input.nSize, AXCL_MEMCPY_HOST_TO_DEVICE, _devid);
}

int ax_runner_axcl::sync_output(int grpid, int idx)
{
    auto &output = get_output(grpid, idx);
    return axcl_Memcpy(output.pVirAddr, (void *)output.phyAddr, output.nSize, AXCL_MEMCPY_DEVICE_TO_HOST, _devid);
}

int ax_runner_axcl::sync_output(int grpid, std::string name)
{
    auto &output = get_output(grpid, name);
    return axcl_Memcpy(output.pVirAddr, (void *)output.phyAddr, output.nSize, AXCL_MEMCPY_DEVICE_TO_HOST, _devid);
}

int ax_runner_axcl::set_input(int grpid, int idx, unsigned long long int phy_addr, unsigned long size)
{
    return axcl_EngineSetInputBufferByIndex(m_handle->ios[grpid], idx, (void *)phy_addr, size, _devid);
}
int ax_runner_axcl::set_output(int grpid, int idx, unsigned long long int phy_addr, unsigned long size)
{
    return axcl_EngineSetOutputBufferByIndex(m_handle->ios[grpid], idx, (void *)phy_addr, size, _devid);
}

int ax_runner_axcl::set_input(int grpid, std::string name, unsigned long long int phy_addr, unsigned long size)
{
    return axcl_EngineSetInputBufferByIndex(m_handle->ios[grpid], get_input(grpid, name).nIdx, (void *)phy_addr, size, _devid);
}

int ax_runner_axcl::set_output(int grpid, std::string name, unsigned long long int phy_addr, unsigned long size)
{
    return axcl_EngineSetOutputBufferByIndex(m_handle->ios[grpid], get_output(grpid, name).nIdx, (void *)phy_addr, size, _devid);
}

int ax_runner_axcl::inference()
{
    return inference(0);
}

int ax_runner_axcl::inference(int grpid)
{
    if (_auto_sync_before_inference)
        for (size_t i = 0; i < mgroup_input_tensors[grpid].size(); i++)
            axcl_Memcpy((void *)mgroup_input_tensors[grpid][i].phyAddr, mgroup_input_tensors[grpid][i].pVirAddr, mgroup_input_tensors[grpid][i].nSize, AXCL_MEMCPY_HOST_TO_DEVICE, _devid);

    auto ret = axcl_EngineExecute(m_handle->handle, m_handle->context, grpid, m_handle->ios[grpid], _devid);
    if (ret != 0)
    {
        fprintf(stderr, "axclrtEngineExecute failed. ret=0x%x\n", ret);
        return ret;
    }
    if (_auto_sync_after_inference)
        for (size_t i = 0; i < mgroup_output_tensors[grpid].size(); i++)
            axcl_Memcpy(mgroup_output_tensors[grpid][i].pVirAddr, (void *)mgroup_output_tensors[grpid][i].phyAddr, mgroup_output_tensors[grpid][i].nSize, AXCL_MEMCPY_DEVICE_TO_HOST, _devid);
    return 0;
}