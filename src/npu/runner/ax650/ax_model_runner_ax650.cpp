#include "ax_model_runner_ax650.hpp"
#include "logger.hpp"
#include <string.h>
#include <fstream>
#include <memory>
#include <mutex>
#include <fcntl.h>
#include <sys/mman.h>
#include <ax_sys_api.h>
#include <ax_engine_api.h>


#define AX_CMM_ALIGN_SIZE 128

#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) ((((x) + ((align) - 1)) / (align)) * (align))
#endif

const char *AX_CMM_SESSION_NAME = "npu";

typedef enum
{
    AX_ENGINE_ABST_DEFAULT = 0,
    AX_ENGINE_ABST_CACHED = 1,
} AX_ENGINE_ALLOC_BUFFER_STRATEGY_T;

typedef std::pair<AX_ENGINE_ALLOC_BUFFER_STRATEGY_T, AX_ENGINE_ALLOC_BUFFER_STRATEGY_T> INPUT_OUTPUT_ALLOC_STRATEGY;

namespace {

// AX_ENGINE is process-global and must be initialized before CreateHandle on some SDK versions.
// Manage it with a ref-count to support multiple pipelines/models safely.
std::mutex g_engine_mu;
int g_engine_refcnt = 0;

int EngineAcquire() {
    std::lock_guard<std::mutex> lock(g_engine_mu);
    if (g_engine_refcnt == 0) {
        AX_ENGINE_NPU_ATTR_T npu_attr;
        memset(&npu_attr, 0, sizeof(npu_attr));
        npu_attr.eHardMode = AX_ENGINE_VIRTUAL_NPU_DISABLE;
        const int ret = AX_ENGINE_Init(&npu_attr);
        if (ret != 0) {
            ALOGE("AX_ENGINE_Init failed ret=%d", ret);
            return ret;
        }
    }
    ++g_engine_refcnt;
    return 0;
}

void EngineRelease() {
    std::lock_guard<std::mutex> lock(g_engine_mu);
    if (g_engine_refcnt <= 0) {
        return;
    }
    --g_engine_refcnt;
    if (g_engine_refcnt == 0) {
        AX_ENGINE_Deinit();
    }
}

}  // namespace

static bool file_exist(const std::string &path)
{
    auto flag = false;

    std::fstream fs(path, std::ios::in | std::ios::binary);
    flag = fs.is_open();
    fs.close();

    return flag;
}

void print_io_info(std::vector<ax_runner_tensor_t> &input, std::vector<ax_runner_tensor_t> &output);

class MMap
{
private:
    void *_add;
    int _size;

public:
    MMap() {}
    MMap(const char *file)
    {
        _add = _mmap(file, &_size);
    }
    ~MMap()
    {
        munmap(_add, _size);
    }

    size_t size()
    {
        return _size;
    }

    void *data()
    {
        return _add;
    }

    static void *_mmap(const char *model_file, int *model_size)
    {
        auto *file_fp = fopen(model_file, "r");
        if (!file_fp)
        {
            ALOGE("Read Run-Joint model(%s) file failed.\n", model_file);
            return nullptr;
        }
        fseek(file_fp, 0, SEEK_END);
        *model_size = ftell(file_fp);
        fclose(file_fp);
        int fd = open(model_file, O_RDWR, 0644);
        void *mmap_add = mmap(NULL, *model_size, PROT_WRITE, MAP_SHARED, fd, 0);
        return mmap_add;
    }
};

static bool read_file(const std::string &path, std::vector<char> &data)
{
    std::fstream fs(path, std::ios::in | std::ios::binary);

    if (!fs.is_open())
    {
        return false;
    }

    fs.seekg(std::ios::end);
    auto fs_end = fs.tellg();
    fs.seekg(std::ios::beg);
    auto fs_beg = fs.tellg();

    auto file_size = static_cast<size_t>(fs_end - fs_beg);
    auto vector_size = data.size();

    data.reserve(vector_size + file_size);
    data.insert(data.end(), std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());

    fs.close();

    return true;
}

void free_io_index(AX_ENGINE_IO_BUFFER_T *io_buf, int index)
{
    for (int i = 0; i < index; ++i)
    {
        AX_ENGINE_IO_BUFFER_T *pBuf = io_buf + i;
        AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
    }
}

void free_io(AX_ENGINE_IO_T *io)
{
    for (size_t j = 0; j < io->nInputSize; ++j)
    {
        AX_ENGINE_IO_BUFFER_T *pBuf = io->pInputs + j;
        AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
    }
    for (size_t j = 0; j < io->nOutputSize; ++j)
    {
        AX_ENGINE_IO_BUFFER_T *pBuf = io->pOutputs + j;
        AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
    }
    delete[] io->pInputs;
    delete[] io->pOutputs;
}

// static inline int prepare_io(AX_ENGINE_IO_INFO_T *info, AX_ENGINE_IO_T *io_data, ax_imgproc_t *pimgproc, INPUT_OUTPUT_ALLOC_STRATEGY strategy)
// {
//     memset(io_data, 0, sizeof(*io_data));
//     io_data->pInputs = new AX_ENGINE_IO_BUFFER_T[info->nInputSize];
//     io_data->nInputSize = info->nInputSize;

//     auto ret = 0;
//     if (info->nInputSize == 1)
//     {
//         auto buffer = &io_data->pInputs[0];
//         buffer->pVirAddr = pimgproc->get()->pVir;
//         buffer->phyAddr = pimgproc->get()->pPhy;
//     }
//     else
//     {
//         ALOGE("Only single input was accepted(got %u).\n", info->nInputSize);
//         // for (uint i = 0; i < info->nInputSize; ++i)
//         // {
//         //     auto meta = info->pInputs[i];
//         //     auto buffer = &io_data->pInputs[i];
//         //     if (strategy.first == AX_ENGINE_ABST_CACHED)
//         //     {
//         //         ret = AX_SYS_MemAllocCached((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
//         //     }
//         //     else
//         //     {
//         //         ret = AX_SYS_MemAlloc((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
//         //     }

//         //     if (ret != 0)
//         //     {
//         //         free_io_index(io_data->pInputs, i);
//         //         fprintf(stderr, "Allocate input{%d} { phy: %p, vir: %p, size: %lu Bytes }. fail \n", i, (void *)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
//         //         return ret;
//         //     }
//         //     // fprintf(stderr, "Allocate input{%d} { phy: %p, vir: %p, size: %lu Bytes }. \n", i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
//         // }
//     }

//     io_data->pOutputs = new AX_ENGINE_IO_BUFFER_T[info->nOutputSize];
//     io_data->nOutputSize = info->nOutputSize;
//     for (uint i = 0; i < info->nOutputSize; ++i)
//     {
//         auto meta = info->pOutputs[i];
//         auto buffer = &io_data->pOutputs[i];
//         buffer->nSize = meta.nSize;

//         ret = AX_SYS_MemAllocCached((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));

//         if (ret != 0)
//         {
//             fprintf(stderr, "Allocate output{%d} { phy: %p, vir: %p, size: %lu Bytes }. fail \n", i, (void *)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
//             free_io_index(io_data->pInputs, io_data->nInputSize);
//             free_io_index(io_data->pOutputs, i);
//             return ret;
//         }
//         // fprintf(stderr, "Allocate output{%d} { phy: %p, vir: %p, size: %lu Bytes }.\n", i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
//     }

//     return 0;
// }

static inline int prepare_io(AX_ENGINE_IO_INFO_T *info, AX_ENGINE_IO_T *io_data, INPUT_OUTPUT_ALLOC_STRATEGY strategy)
{
    memset(io_data, 0, sizeof(*io_data));
    io_data->pInputs = new AX_ENGINE_IO_BUFFER_T[info->nInputSize];
    io_data->nInputSize = info->nInputSize;

    auto ret = 0;
    for (uint i = 0; i < info->nInputSize; ++i)
    {
        auto meta = info->pInputs[i];
        auto buffer = &io_data->pInputs[i];
        if (strategy.first == AX_ENGINE_ABST_CACHED)
        {
            ret = AX_SYS_MemAllocCached((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
        }
        else
        {
            ret = AX_SYS_MemAlloc((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
        }

        if (ret != 0)
        {
            free_io_index(io_data->pInputs, i);
            fprintf(stderr, "Allocate input{%d} { phy: %p, vir: %p, size: %lu Bytes }. fail \n", i, (void *)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
            return ret;
        }
        memset(buffer->pVirAddr, 0, meta.nSize);
        // fprintf(stderr, "Allocate input{%d} { phy: %p, vir: %p, size: %lu Bytes }. \n", i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
    }

    io_data->pOutputs = new AX_ENGINE_IO_BUFFER_T[info->nOutputSize];
    io_data->nOutputSize = info->nOutputSize;
    for (uint i = 0; i < info->nOutputSize; ++i)
    {
        auto meta = info->pOutputs[i];
        auto buffer = &io_data->pOutputs[i];
        buffer->nSize = meta.nSize;
        if (strategy.second == AX_ENGINE_ABST_CACHED)
        {
            ret = AX_SYS_MemAllocCached((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
        }
        else
        {
            ret = AX_SYS_MemAlloc((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
        }
        if (ret != 0)
        {
            fprintf(stderr, "Allocate output{%d} { phy: %p, vir: %p, size: %lu Bytes }. fail \n", i, (void *)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
            free_io_index(io_data->pInputs, io_data->nInputSize);
            free_io_index(io_data->pOutputs, i);
            return ret;
        }
        memset(buffer->pVirAddr, 0, meta.nSize);
        // fprintf(stderr, "Allocate output{%d} { phy: %p, vir: %p, size: %lu Bytes }.\n", i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
    }

    return 0;
}

struct ax_joint_runner_ax650_handle_t
{
    AX_ENGINE_HANDLE handle;
    AX_ENGINE_CONTEXT_T context;
    std::vector<AX_ENGINE_IO_INFO_T *> io_info;
    std::vector<AX_ENGINE_IO_T> io_data;

    int algo_width, algo_height;
    int algo_colorformat;
};

int ax_runner_ax650::init(const void *model_data, unsigned int model_size, int devid)
{
    if (m_handle)
    {
        return -1;
    }
    m_handle = new ax_joint_runner_ax650_handle_t;
    _devid = devid;
    int ret;

    ret = EngineAcquire();
    if (ret != 0) {
        delete m_handle;
        m_handle = nullptr;
        return ret;
    }

    // 3. create handle

    ret = AX_ENGINE_CreateHandle(&m_handle->handle, model_data, model_size);
    if (0 != ret)
    {
        ALOGE("AX_ENGINE_CreateHandle");
        delete m_handle;
        m_handle = nullptr;
        EngineRelease();
        return ret;
    }
    // fprintf(stdout, "Engine creating handle is done.\n");

    // 4. create context
    ret = AX_ENGINE_CreateContext(m_handle->handle);
    if (0 != ret)
    {
        ALOGE("AX_ENGINE_CreateContext");
        AX_ENGINE_DestroyHandle(m_handle->handle);
        delete m_handle;
        m_handle = nullptr;
        EngineRelease();
        return ret;
    }
    ret = AX_ENGINE_CreateContextV2(m_handle->handle, &m_handle->context);
    if (0 != ret)
    {
        ALOGE("AX_ENGINE_CreateContextV2");
        AX_ENGINE_DestroyHandle(m_handle->handle);
        delete m_handle;
        m_handle = nullptr;
        EngineRelease();
        return ret;
    }
    // fprintf(stdout, "Engine creating context is done.\n");

    // 5. set io
    AX_U32 io_count = 0;
    ret = AX_ENGINE_GetGroupIOInfoCount(m_handle->handle, &io_count);
    if (0 != ret)
    {
        ALOGE("AX_ENGINE_GetGroupIOInfoCount");
        AX_ENGINE_DestroyHandle(m_handle->handle);
        delete m_handle;
        m_handle = nullptr;
        EngineRelease();
        return ret;
    }
    // ALOGI("io_count=%d", io_count);

    m_handle->io_info.resize(io_count);
    m_handle->io_data.resize(io_count);
    mgroup_input_tensors.resize(io_count);
    mgroup_output_tensors.resize(io_count);

    for (int grpid = 0; grpid < io_count; grpid++)
    {
        AX_ENGINE_IO_INFO_T *io_info = nullptr;
        ret = AX_ENGINE_GetGroupIOInfo(m_handle->handle, grpid, &io_info);
        if (0 != ret)
        {
            ALOGE("AX_ENGINE_GetIOInfo");
            AX_ENGINE_DestroyHandle(m_handle->handle);
            delete m_handle;
            m_handle = nullptr;
            EngineRelease();
            return ret;
        }
        // print_io_info(io_info);

        m_handle->io_info[grpid] = io_info;

        ret = prepare_io(m_handle->io_info[grpid], &m_handle->io_data[grpid], std::make_pair(AX_ENGINE_ABST_DEFAULT, AX_ENGINE_ABST_CACHED));
        if (0 != ret)
        {
            ALOGE("prepare_io grpid=%d", grpid);
            AX_ENGINE_DestroyHandle(m_handle->handle);
            delete m_handle;
            m_handle = nullptr;
            EngineRelease();
            return ret;
        }
    }

    for (size_t grpid = 0; grpid < io_count; grpid++)
    {
        auto &io_info = m_handle->io_info[grpid];
        auto &io_data = m_handle->io_data[grpid];
        for (size_t i = 0; i < io_info->nOutputSize; i++)
        {
            ax_runner_tensor_t tensor;
            tensor.nIdx = i;
            tensor.sName = std::string(io_info->pOutputs[i].pName);
            tensor.nSize = io_info->pOutputs[i].nSize;
            for (size_t j = 0; j < io_info->pOutputs[i].nShapeSize; j++)
            {
                tensor.vShape.push_back(io_info->pOutputs[i].pShape[j]);
            }
            // tensor.eColorSpace = ax_color_space_unknown;
            tensor.phyAddr = io_data.pOutputs[i].phyAddr;
            tensor.pVirAddr = io_data.pOutputs[i].pVirAddr;
            mgroup_output_tensors[grpid].push_back(tensor);
        }

        for (size_t i = 0; i < io_info->nInputSize; i++)
        {
            ax_runner_tensor_t tensor;
            tensor.nIdx = i;
            tensor.sName = std::string(io_info->pInputs[i].pName);
            tensor.nSize = io_info->pInputs[i].nSize;
            // switch (io_info->pInputs[i].pExtraMeta->eColorSpace)
            // {
            // case AX_ENGINE_CS_NV12:
            //     tensor.eColorSpace = ax_color_space_nv12;
            //     break;
            // case AX_ENGINE_CS_RGB:
            //     tensor.eColorSpace = ax_color_space_rgb;
            //     break;
            // case AX_ENGINE_CS_BGR:
            //     tensor.eColorSpace = ax_color_space_bgr;
            //     break;
            // default:
            //     ALOGW("unknown model color format");
            //     tensor.eColorSpace = ax_color_space_unknown;
            //     break;
            // }
            for (size_t j = 0; j < io_info->pInputs[i].nShapeSize; j++)
            {
                tensor.vShape.push_back(io_info->pInputs[i].pShape[j]);
            }
            tensor.phyAddr = io_data.pInputs[i].phyAddr;
            tensor.pVirAddr = io_data.pInputs[i].pVirAddr;
            mgroup_input_tensors[grpid].push_back(tensor);
        }

        // print_io_info(mgroup_input_tensors[grpid], mgroup_output_tensors[grpid]);
    }

    moutput_tensors = mgroup_output_tensors[0];
    minput_tensors = mgroup_input_tensors[0];

    // m_imgproc.set(m_handle->io_data.pInputs[0].phyAddr, m_handle->io_data.pInputs[0].pVirAddr);
    // fprintf(stdout, "Engine alloc io is done. \n");

    return ret;
}

void ax_runner_ax650::deinit()
{
    if (m_handle && m_handle->handle)
    {
        for (size_t i = 0; i < m_handle->io_data.size(); i++)
        {
            free_io(&m_handle->io_data[i]);
        }
        AX_ENGINE_DestroyHandle(m_handle->handle);
    }
    delete m_handle;
    m_handle = nullptr;
    EngineRelease();
}

int ax_runner_ax650::set_affinity(int id)
{
    return AX_ENGINE_SetAffinity(m_handle->handle, id);
}

// int ax_runner_ax650::mem_sync_input(int idx)
// {
//     return AX_SYS_MinvalidateCache(minput_tensors[idx].phyAddr, minput_tensors[idx].pVirAddr, minput_tensors[idx].nSize);
// }

// int ax_runner_ax650::mem_sync_output(int idx)
// {
//     return AX_SYS_MinvalidateCache(moutput_tensors[idx].phyAddr, moutput_tensors[idx].pVirAddr, moutput_tensors[idx].nSize);
// }

// int ax_runner_ax650::mem_sync_input(std::string name)
// {
//     auto &tensor = get_input(name);
//     return AX_SYS_MinvalidateCache(tensor.phyAddr, tensor.pVirAddr, tensor.nSize);
// }

// int ax_runner_ax650::mem_sync_output(std::string name)
// {
//     auto &tensor = get_output(name);
//     return AX_SYS_MinvalidateCache(tensor.phyAddr, tensor.pVirAddr, tensor.nSize);
// }

// int ax_runner_ax650::inference(ax_image_t *pstFrame, const ax_bbox_t *crop_resize_box)
// {
//     // if (m_imgproc.process(pstFrame, (ax_bbox_t *)crop_resize_box) != 0)
//     // {
//     //     ALOGE("image process failed");
//     //     return -1;
//     // }

//     // memcpy(minput_tensors[0].pVirAddr, pstFrame->pVir, minput_tensors[0].nSize);
//     return inference();
// }

int ax_runner_ax650::sync_output(int idx)
{
    auto &tensor = get_output(idx);
    AX_SYS_MinvalidateCache(tensor.phyAddr, tensor.pVirAddr, tensor.nSize);
    return 0;
}

int ax_runner_ax650::sync_output(std::string name)
{
    auto &tensor = get_output(name);
    AX_SYS_MinvalidateCache(tensor.phyAddr, tensor.pVirAddr, tensor.nSize);
    return 0;
}

int ax_runner_ax650::sync_output(int grpid, int idx)
{
    auto &tensor = get_output(grpid, idx);
    AX_SYS_MinvalidateCache(tensor.phyAddr, tensor.pVirAddr, tensor.nSize);
    return 0;
}

int ax_runner_ax650::sync_output(int grpid, std::string name)
{
    auto &tensor = get_output(grpid, name);
    AX_SYS_MinvalidateCache(tensor.phyAddr, tensor.pVirAddr, tensor.nSize);
    return 0;
}

int ax_runner_ax650::inference()
{
    int ret = AX_ENGINE_RunSync(m_handle->handle, &m_handle->io_data[0]);
    for (size_t i = 0; i < get_num_outputs(); i++)
    {
        auto &tensor = get_output(i);
        AX_SYS_MinvalidateCache(tensor.phyAddr, tensor.pVirAddr, tensor.nSize);
    }
    return ret;
}
int ax_runner_ax650::inference(int grpid)
{
    int ret = AX_ENGINE_RunGroupIOSync(m_handle->handle, m_handle->context, grpid, &m_handle->io_data[grpid]);

    for (size_t i = 0; i < get_num_outputs(); i++)
    {
        auto &tensor = get_output(grpid, i);
        AX_SYS_MinvalidateCache(tensor.phyAddr, tensor.pVirAddr, tensor.nSize);
    }
    return ret;
}
