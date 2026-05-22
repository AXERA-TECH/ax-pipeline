#include "axcl_manager.h"
#include "ax_cmm_utils.hpp"
#include "logger.hpp"

#include <axcl.h>
#include <axcl_rt_context.h>
#include <axcl_rt_device.h>
#include <axcl_rt_engine.h>

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <mutex>
#include <string>

namespace {

axclrtEngineVNpuKind ParseVnpuKindFromEnv() noexcept {
    const char* env = std::getenv("AXP_AXCL_VNPU_KIND");
    if (!env || !*env) return AXCL_VNPU_DISABLE;
    std::string s(env);
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (s == "0" || s == "disable" || s == "off" || s == "false") return AXCL_VNPU_DISABLE;
    if (s == "1" || s == "enable" || s == "on" || s == "true" || s == "std" || s == "standard") return AXCL_VNPU_ENABLE;
    if (s == "2" || s == "big_little") return AXCL_VNPU_BIG_LITTLE;
    if (s == "3" || s == "little_big") return AXCL_VNPU_LITTLE_BIG;
    return AXCL_VNPU_DISABLE;
}

struct ThreadContext {
    int device_id = -1;
    int runtime_id = -1;
    axclrtContext ctx = nullptr;

    ~ThreadContext() {
        if (ctx != nullptr) {
            (void)axclrtDestroyContext(ctx);
            ctx = nullptr;
        }
        device_id = -1;
        runtime_id = -1;
    }
};

thread_local ThreadContext g_thread_ctx;

bool EnsureThreadContext(int device_id) noexcept {
    axclrtDeviceList lst{};
    if (const auto ret = axclrtGetDeviceList(&lst); ret != AXCL_SUCC || lst.num == 0) {
        ALOGE("axclrtGetDeviceList failed{0x%8x}, total=%u.", ret, lst.num);
        return false;
    }
    if (device_id < 0 || device_id >= static_cast<int>(lst.num)) {
        ALOGE("Invalid AXCL device index %d, total=%u.", device_id, lst.num);
        return false;
    }

    const int runtime_id = lst.devices[device_id];
    if (g_thread_ctx.ctx != nullptr && g_thread_ctx.device_id == device_id) {
        // Best-effort re-bind.
        (void)axclrtSetDevice(runtime_id);
        (void)axclrtSetCurrentContext(g_thread_ctx.ctx);
        return true;
    }

    if (g_thread_ctx.ctx != nullptr) {
        (void)axclrtDestroyContext(g_thread_ctx.ctx);
        g_thread_ctx.ctx = nullptr;
        g_thread_ctx.device_id = -1;
        g_thread_ctx.runtime_id = -1;
    }

    if (const auto ret = axclrtSetDevice(runtime_id); ret != AXCL_SUCC) {
        ALOGE("axclrtSetDevice failed{0x%8x} runtime_id=%d.", ret, runtime_id);
        return false;
    }

    axclrtContext ctx = nullptr;
    if (const auto ret = axclrtCreateContext(&ctx, runtime_id); ret != AXCL_SUCC || ctx == nullptr) {
        ALOGE("axclrtCreateContext failed{0x%8x}.", ret);
        return false;
    }
    if (const auto ret = axclrtSetCurrentContext(ctx); ret != AXCL_SUCC) {
        ALOGE("axclrtSetCurrentContext failed{0x%8x}.", ret);
        (void)axclrtDestroyContext(ctx);
        return false;
    }

    g_thread_ctx.ctx = ctx;
    g_thread_ctx.device_id = device_id;
    g_thread_ctx.runtime_id = runtime_id;
    return true;
}

std::once_flag g_engine_once;
std::atomic<bool> g_engine_ok{false};
std::atomic<int> g_engine_kind{-1};

bool EnsureEngineInit(int device_id) noexcept {
    if (!EnsureThreadContext(device_id)) {
        return false;
    }

    std::call_once(g_engine_once, [&]() {
        axclrtEngineVNpuKind vnpu = ParseVnpuKindFromEnv();
        axclError ret = axclrtEngineInit(vnpu);
        if (ret != AXCL_SUCC && vnpu != AXCL_VNPU_DISABLE) {
            ALOGE("axclrtEngineInit(vnpu=%d) failed{0x%8x}, fallback to VNPU_DISABLE.", static_cast<int>(vnpu), ret);
            vnpu = AXCL_VNPU_DISABLE;
            ret = axclrtEngineInit(vnpu);
        }
        if (ret != AXCL_SUCC) {
            ALOGE("axclrtEngineInit(vnpu=%d) failed{0x%8x}.", static_cast<int>(vnpu), ret);
            g_engine_ok.store(false);
            g_engine_kind.store(static_cast<int>(vnpu));
            return;
        }

        axclrtEngineVNpuKind got{};
        if (axclrtEngineGetVNpuKind(&got) == AXCL_SUCC) {
            ALOGI("AXCL engine VNPU kind=%d", static_cast<int>(got));
            g_engine_kind.store(static_cast<int>(got));
        } else {
            g_engine_kind.store(static_cast<int>(vnpu));
        }
        g_engine_ok.store(true);
    });

    return g_engine_ok.load();
}

}  // namespace

axclError axcl_Init() {
    return axclInit(0);
}

axclError axcl_Finalize() {
    // Note: engine finalization is process-global; leaving it to process exit is usually fine.
    return axclFinalize();
}

axclError axcl_Dev_Init(int devid) {
    return EnsureEngineInit(devid) ? AXCL_SUCC : -1;
}

bool axcl_Dev_IsInit(int /*devid*/) {
    return g_engine_ok.load();
}

axclError axcl_Dev_Exit(int /*devid*/) {
    // Thread contexts are released via thread_local destructors.
    return AXCL_SUCC;
}

int axcl_GetCMMRemain(int devid) {
    // Keep behavior: helper reads driver debug output; it does not require a bound runtime context.
    return get_pcie_remaining_cmm_size(devid);
}

axclError axcl_Malloc(void **devPtr, size_t size, axclrtMemMallocPolicy policy, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtMalloc(devPtr, size, policy);
}

axclError axcl_MallocCached(void **devPtr, size_t size, axclrtMemMallocPolicy policy, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtMallocCached(devPtr, size, policy);
}

axclError axcl_Free(void *devPtr, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtFree(devPtr);
}

axclError axcl_MemFlush(void *devPtr, size_t size, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtMemFlush(devPtr, size);
}

axclError axcl_MemInvalidate(void *devPtr, size_t size, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtMemInvalidate(devPtr, size);
}

axclError axcl_MallocHost(void **hostPtr, size_t size, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtMallocHost(hostPtr, size);
}

axclError axcl_FreeHost(void *hostPtr, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtFreeHost(hostPtr);
}

axclError axcl_Memset(void *devPtr, uint8_t value, size_t count, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtMemset(devPtr, value, count);
}

axclError axcl_Memcpy(void *dstPtr, const void *srcPtr, size_t count, axclrtMemcpyKind kind, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtMemcpy(dstPtr, srcPtr, count, kind);
}

axclError axcl_Memcmp(const void *devPtr1, const void *devPtr2, size_t count, int devid) {
    if (!EnsureThreadContext(devid)) return -1;
    return axclrtMemcmp(devPtr1, devPtr2, count);
}

axclError axcl_EngineLoadFromFile(const char *modelPath, uint64_t *modelId, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineLoadFromFile(modelPath, modelId);
}

axclError axcl_EngineLoadFromMem(const void *model, uint64_t modelSize, uint64_t *modelId, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineLoadFromMem(model, modelSize, modelId);
}

axclError axcl_EngineUnload(uint64_t modelId, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineUnload(modelId);
}

const char* axcl_EngineGetModelCompilerVersion(uint64_t modelId, int devid) {
    if (!EnsureEngineInit(devid)) return nullptr;
    return axclrtEngineGetModelCompilerVersion(modelId);
}

axclError axcl_EngineSetAffinity(uint64_t modelId, axclrtEngineSet set, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineSetAffinity(modelId, set);
}

axclError axcl_EngineGetAffinity(uint64_t modelId, axclrtEngineSet *set, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetAffinity(modelId, set);
}

axclError axcl_EngineGetUsage(const char *modelPath, int64_t *sysSize, int64_t *cmmSize, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetUsage(modelPath, sysSize, cmmSize);
}

axclError axcl_EngineGetUsageFromMem(const void *model, uint64_t modelSize, int64_t *sysSize, int64_t *cmmSize, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetUsageFromMem(model, modelSize, sysSize, cmmSize);
}

axclError axcl_EngineGetUsageFromModelId(uint64_t modelId, int64_t *sysSize, int64_t *cmmSize, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetUsageFromModelId(modelId, sysSize, cmmSize);
}

axclError axcl_EngineGetModelType(const char *modelPath, axclrtEngineModelKind *modelType, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetModelType(modelPath, modelType);
}

axclError axcl_EngineGetModelTypeFromMem(const void *model, uint64_t modelSize, axclrtEngineModelKind *modelType, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetModelTypeFromMem(model, modelSize, modelType);
}

axclError axcl_EngineGetModelTypeFromModelId(uint64_t modelId, axclrtEngineModelKind *modelType, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetModelTypeFromModelId(modelId, modelType);
}

axclError axcl_EngineGetIOInfo(uint64_t modelId, axclrtEngineIOInfo *ioInfo, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetIOInfo(modelId, ioInfo);
}

axclError axcl_EngineDestroyIOInfo(axclrtEngineIOInfo ioInfo, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineDestroyIOInfo(ioInfo);
}

axclError axcl_EngineGetShapeGroupsCount(axclrtEngineIOInfo ioInfo, int32_t *count, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetShapeGroupsCount(ioInfo, count);
}

uint32_t axcl_EngineGetNumInputs(axclrtEngineIOInfo ioInfo, int devid) {
    if (!EnsureEngineInit(devid)) return 0;
    return axclrtEngineGetNumInputs(ioInfo);
}

uint32_t axcl_EngineGetNumOutputs(axclrtEngineIOInfo ioInfo, int devid) {
    if (!EnsureEngineInit(devid)) return 0;
    return axclrtEngineGetNumOutputs(ioInfo);
}

uint64_t axcl_EngineGetInputSizeByIndex(axclrtEngineIOInfo ioInfo, uint32_t group, uint32_t index, int devid) {
    if (!EnsureEngineInit(devid)) return 0;
    return axclrtEngineGetInputSizeByIndex(ioInfo, group, index);
}

uint64_t axcl_EngineGetOutputSizeByIndex(axclrtEngineIOInfo ioInfo, uint32_t group, uint32_t index, int devid) {
    if (!EnsureEngineInit(devid)) return 0;
    return axclrtEngineGetOutputSizeByIndex(ioInfo, group, index);
}

const char* axcl_EngineGetInputNameByIndex(axclrtEngineIOInfo ioInfo, uint32_t index, int devid) {
    if (!EnsureEngineInit(devid)) return nullptr;
    return axclrtEngineGetInputNameByIndex(ioInfo, index);
}

const char* axcl_EngineGetOutputNameByIndex(axclrtEngineIOInfo ioInfo, uint32_t index, int devid) {
    if (!EnsureEngineInit(devid)) return nullptr;
    return axclrtEngineGetOutputNameByIndex(ioInfo, index);
}

int32_t axcl_EngineGetInputIndexByName(axclrtEngineIOInfo ioInfo, const char *name, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetInputIndexByName(ioInfo, name);
}

int32_t axcl_EngineGetOutputIndexByName(axclrtEngineIOInfo ioInfo, const char *name, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetOutputIndexByName(ioInfo, name);
}

axclError axcl_EngineGetInputDims(axclrtEngineIOInfo ioInfo, uint32_t group, uint32_t index, axclrtEngineIODims *dims, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetInputDims(ioInfo, group, index, dims);
}

axclError axcl_EngineGetOutputDims(axclrtEngineIOInfo ioInfo, uint32_t group, uint32_t index, axclrtEngineIODims *dims, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetOutputDims(ioInfo, group, index, dims);
}

axclError axcl_EngineCreateIO(axclrtEngineIOInfo ioInfo, axclrtEngineIO *io, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineCreateIO(ioInfo, io);
}

axclError axcl_EngineDestroyIO(axclrtEngineIO io, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineDestroyIO(io);
}

axclError axcl_EngineSetInputBufferByIndex(axclrtEngineIO io, uint32_t index, const void *dataBuffer, uint64_t size, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineSetInputBufferByIndex(io, index, dataBuffer, size);
}

axclError axcl_EngineSetOutputBufferByIndex(axclrtEngineIO io, uint32_t index, const void *dataBuffer, uint64_t size, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineSetOutputBufferByIndex(io, index, dataBuffer, size);
}

axclError axcl_EngineSetInputBufferByName(axclrtEngineIO io, const char *name, const void *dataBuffer, uint64_t size, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineSetInputBufferByName(io, name, dataBuffer, size);
}

axclError axcl_EngineSetOutputBufferByName(axclrtEngineIO io, const char *name, const void *dataBuffer, uint64_t size, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineSetOutputBufferByName(io, name, dataBuffer, size);
}

axclError axcl_EngineGetInputBufferByIndex(axclrtEngineIO io, uint32_t index, void **dataBuffer, uint64_t *size, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetInputBufferByIndex(io, index, dataBuffer, size);
}

axclError axcl_EngineGetOutputBufferByIndex(axclrtEngineIO io, uint32_t index, void **dataBuffer, uint64_t *size, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetOutputBufferByIndex(io, index, dataBuffer, size);
}

axclError axcl_EngineGetInputBufferByName(axclrtEngineIO io, const char *name, void **dataBuffer, uint64_t *size, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetInputBufferByName(io, name, dataBuffer, size);
}

axclError axcl_EngineGetOutputBufferByName(axclrtEngineIO io, const char *name, void **dataBuffer, uint64_t *size, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineGetOutputBufferByName(io, name, dataBuffer, size);
}

axclError axcl_EngineSetDynamicBatchSize(axclrtEngineIO io, uint32_t batchSize, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineSetDynamicBatchSize(io, batchSize);
}

axclError axcl_EngineCreateContext(uint64_t modelId, uint64_t *contextId, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineCreateContext(modelId, contextId);
}

axclError axcl_EngineExecute(uint64_t modelId, uint64_t contextId, uint32_t group, axclrtEngineIO io, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineExecute(modelId, contextId, group, io);
}

axclError axcl_EngineExecuteAsync(uint64_t modelId, uint64_t contextId, uint32_t group, axclrtEngineIO io, axclrtStream stream, int devid) {
    if (!EnsureEngineInit(devid)) return -1;
    return axclrtEngineExecuteAsync(modelId, contextId, group, io, stream);
}
