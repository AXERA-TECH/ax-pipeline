#include "ax_plugin/ax_plugin_process.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "ax_plugin/ax_plugin.h"
#include "ax_plugin/ax_plugin_ipc.hpp"

#if defined(__linux__) || defined(__unix__)
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

#if defined(AXPIPELINE_APP_PLATFORM_AXCL)
#  include <axcl.h>
#endif

namespace axpipeline::plugin {

namespace {

std::uint64_t NowMs() noexcept {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

#if defined(__linux__) || defined(__unix__)

bool ReadExact(int fd, void* buf, std::size_t n) {
    std::uint8_t* p = static_cast<std::uint8_t*>(buf);
    std::size_t left = n;
    while (left > 0) {
        const ssize_t r = ::read(fd, p, left);
        if (r == 0) {
            return false;  // EOF
        }
        if (r < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += static_cast<std::size_t>(r);
        left -= static_cast<std::size_t>(r);
    }
    return true;
}

bool WriteExact(int fd, const void* buf, std::size_t n) {
    const std::uint8_t* p = static_cast<const std::uint8_t*>(buf);
    std::size_t left = n;
    while (left > 0) {
        const ssize_t w = ::write(fd, p, left);
        if (w < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += static_cast<std::size_t>(w);
        left -= static_cast<std::size_t>(w);
    }
    return true;
}

std::string GetExeDir() {
    // Linux: /proc/self/exe is reliable for installed/built binaries.
#  if defined(__linux__)
    char path[4096] = {};
    const ssize_t n = ::readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (n <= 0) return ".";
    path[n] = '\0';
    std::string s(path);
    const auto pos = s.find_last_of('/');
    if (pos == std::string::npos) return ".";
    return s.substr(0, pos);
#  else
    return ".";
#  endif
}

ax_plugin_pixel_format_e ToPluginFormat(axvsdk::common::PixelFormat fmt) noexcept {
    switch (fmt) {
    case axvsdk::common::PixelFormat::kNv12:
        return AX_PLUGIN_PIXEL_FORMAT_NV12;
    case axvsdk::common::PixelFormat::kRgb24:
        return AX_PLUGIN_PIXEL_FORMAT_RGB24;
    case axvsdk::common::PixelFormat::kBgr24:
        return AX_PLUGIN_PIXEL_FORMAT_BGR24;
    case axvsdk::common::PixelFormat::kUnknown:
    default:
        return AX_PLUGIN_PIXEL_FORMAT_UNKNOWN;
    }
}

std::uint32_t PlaneCountForFormat(axvsdk::common::PixelFormat fmt) noexcept {
    switch (fmt) {
    case axvsdk::common::PixelFormat::kNv12:
        return 2U;
    case axvsdk::common::PixelFormat::kRgb24:
    case axvsdk::common::PixelFormat::kBgr24:
        return 1U;
    case axvsdk::common::PixelFormat::kUnknown:
    default:
        return 0U;
    }
}

bool SerializeImageToHostPayload(const axvsdk::common::AxImage& image,
                                 std::int32_t device_id,
                                 std::vector<std::uint8_t>* payload,
                                 std::string* error) {
    if (payload == nullptr) return false;
    payload->clear();

    const auto fmt = image.format();
    const auto pf = ToPluginFormat(fmt);
    if (pf == AX_PLUGIN_PIXEL_FORMAT_UNKNOWN) {
        if (error) *error = "unsupported image format";
        return false;
    }

    const std::uint32_t expected_planes = PlaneCountForFormat(fmt);
    const std::uint32_t planes = static_cast<std::uint32_t>(image.plane_count());
    if (expected_planes != 0 && planes < expected_planes) {
        if (error) *error = "invalid plane_count";
        return false;
    }

    ipc::InferReq req{};
    req.format = static_cast<std::uint32_t>(pf);
    req.width = image.width();
    req.height = image.height();
    req.plane_count = planes;
    for (std::size_t i = 0; i < 3; ++i) {
        req.strides[i] = 0;
        req.plane_sizes[i] = 0;
    }

    std::size_t total_bytes = 0;
    const std::size_t pc = std::min<std::size_t>(image.plane_count(), 3U);
    for (std::size_t i = 0; i < pc; ++i) {
        const std::size_t sz = image.plane_size(i);
        req.strides[i] = static_cast<std::uint64_t>(image.stride(i));
        req.plane_sizes[i] = static_cast<std::uint64_t>(sz);
        total_bytes += sz;
    }

    payload->resize(sizeof(ipc::InferReq) + total_bytes);
    std::memcpy(payload->data(), &req, sizeof(req));

#if defined(AXPIPELINE_APP_PLATFORM_AXCL)
    // For AXCL, device_id is treated as device *index* (0..N-1). We must resolve it to runtime device ID
    // and bind a per-thread AXCL context before calling axclrtMemcpy.
    struct AxclThreadCtx {
        std::int32_t device_index{-1};
        std::int32_t runtime_device_id{-1};
        axclrtContext context{nullptr};
    };
    static thread_local AxclThreadCtx s_axcl_ctx;

    auto ensure_axcl_ctx = [&](std::int32_t index, std::string* err) -> bool {
        if (index < 0) index = 0;

        axclrtDeviceList lst{};
        const auto gl = axclrtGetDeviceList(&lst);
        if (gl != AXCL_SUCC || lst.num == 0) {
            if (err) *err = "axclrtGetDeviceList failed or no device";
            return false;
        }
        if (static_cast<std::uint32_t>(index) >= lst.num) {
            if (err) *err = "invalid AXCL device index: " + std::to_string(index) + " total=" + std::to_string(lst.num);
            return false;
        }
        const std::int32_t runtime_id = lst.devices[index];

        if (s_axcl_ctx.context != nullptr && s_axcl_ctx.device_index == index) {
            axclrtContext current = nullptr;
            if (axclrtGetCurrentContext(&current) == AXCL_SUCC && current == s_axcl_ctx.context) {
                return true;
            }
            (void)axclrtSetCurrentContext(s_axcl_ctx.context);
            return true;
        }

        if (s_axcl_ctx.context != nullptr) {
            (void)axclrtDestroyContext(s_axcl_ctx.context);
            s_axcl_ctx.context = nullptr;
            s_axcl_ctx.device_index = -1;
            s_axcl_ctx.runtime_device_id = -1;
        }

        if (axclrtSetDevice(runtime_id) != AXCL_SUCC) {
            if (err) *err = "axclrtSetDevice failed, runtime_id=" + std::to_string(runtime_id);
            return false;
        }

        axclrtContext ctx = nullptr;
        const auto cr = axclrtCreateContext(&ctx, runtime_id);
        if (cr != AXCL_SUCC || ctx == nullptr) {
            if (err) *err = "axclrtCreateContext failed, runtime_id=" + std::to_string(runtime_id);
            return false;
        }
        const auto sr = axclrtSetCurrentContext(ctx);
        if (sr != AXCL_SUCC) {
            (void)axclrtDestroyContext(ctx);
            if (err) *err = "axclrtSetCurrentContext failed, runtime_id=" + std::to_string(runtime_id);
            return false;
        }

        s_axcl_ctx.context = ctx;
        s_axcl_ctx.device_index = index;
        s_axcl_ctx.runtime_device_id = runtime_id;
        return true;
    };
#endif

    std::size_t off = sizeof(ipc::InferReq);
    for (std::size_t i = 0; i < pc; ++i) {
        const std::size_t sz = static_cast<std::size_t>(req.plane_sizes[i]);
        if (sz == 0) continue;

        void* dst = payload->data() + off;

#if defined(AXPIPELINE_APP_PLATFORM_AXCL)
        // AXCL: input image may be device-side and not directly CPU readable.
        if (image.memory_type() != axvsdk::common::MemoryType::kExternal) {
            std::string ctx_err;
            if (!ensure_axcl_ctx(device_id, &ctx_err)) {
                if (error) *error = ctx_err.empty() ? "ensure AXCL context failed" : ctx_err;
                return false;
            }
            const auto phy = image.physical_address(i);
            const void* src = (phy != 0) ? reinterpret_cast<const void*>(static_cast<std::uintptr_t>(phy))
                                         : image.virtual_address(i);
            if (src == nullptr) {
                if (error) *error = "AXCL device plane address is null";
                return false;
            }
            const auto ret = axclrtMemcpy(dst, src, sz, AXCL_MEMCPY_DEVICE_TO_HOST);
            if (ret != 0) {
                if (error) *error = "axclrtMemcpy(D2H image) failed: ret=" + std::to_string(ret);
                return false;
            }
            off += sz;
            continue;
        }
#endif

        const auto* src = image.plane_data(i);
        if (src == nullptr) {
            if (error) *error = "image plane not accessible on host";
            return false;
        }
        std::memcpy(dst, src, sz);
        off += sz;
    }

    return true;
}

#endif  // __linux__ || __unix__

}  // namespace

AxPluginProcess::~AxPluginProcess() {
    Close();
}

bool AxPluginProcess::Open(const std::string& so_path,
                           const std::string& init_json,
                           std::int32_t device_id,
                           std::string* error) {
    Close();
    so_path_ = so_path;
    init_json_ = init_json;
    device_id_ = device_id;

#if !(defined(__linux__) || defined(__unix__))
    if (error) *error = "subprocess plugin is only supported on unix";
    return false;
#else
    if (so_path_.empty()) {
        if (error) *error = "plugin path is empty";
        return false;
    }
    if (!Spawn(error)) {
        return false;
    }
    if (!SendInit(error)) {
        Close();
        return false;
    }
    return true;
#endif
}

void AxPluginProcess::KillChild() noexcept {
#if defined(__linux__) || defined(__unix__)
    if (child_pid_ > 0) {
        ::kill(child_pid_, SIGKILL);
        (void)::waitpid(child_pid_, nullptr, 0);
    }
    child_pid_ = -1;
#endif
}

void AxPluginProcess::Close() noexcept {
#if defined(__linux__) || defined(__unix__)
    if (fd_ >= 0) {
        // Best-effort shutdown request.
        ipc::MsgHeader h{};
        h.magic = ipc::kMagic;
        h.version = ipc::kVersion;
        h.type = static_cast<std::uint16_t>(ipc::MsgType::kShutdownReq);
        h.payload_size = sizeof(ipc::ShutdownReq);
        ipc::ShutdownReq req{};
        (void)WriteExact(fd_, &h, sizeof(h));
        (void)WriteExact(fd_, &req, sizeof(req));
    }
    if (fd_ >= 0) {
        ::close(fd_);
    }
    fd_ = -1;
    KillChild();
#endif

    so_path_.clear();
    init_json_.clear();
    device_id_ = -1;
    restart_failures_ = 0;
    last_restart_ms_ = 0;
}

#if defined(__linux__) || defined(__unix__)

bool AxPluginProcess::Spawn(std::string* error) {
    int sv[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        if (error) *error = "socketpair failed";
        return false;
    }

    // Parent keeps sv[0]. Child will inherit sv[1] via exec.
    (void)::fcntl(sv[0], F_SETFD, FD_CLOEXEC);
    int flags = ::fcntl(sv[1], F_GETFD);
    if (flags >= 0) {
        flags &= ~FD_CLOEXEC;
        (void)::fcntl(sv[1], F_SETFD, flags);
    }

    const std::string exe_dir = GetExeDir();
    const std::string host_path = exe_dir + "/ax_plugin_host";
    const std::string arg_fd = "--ipc-fd=" + std::to_string(sv[1]);

    const pid_t pid = ::fork();
    if (pid < 0) {
        ::close(sv[0]);
        ::close(sv[1]);
        if (error) *error = "fork failed";
        return false;
    }
    if (pid == 0) {
        // Child: close parent end; exec host.
        ::close(sv[0]);
        char* const argv[] = {
            const_cast<char*>(host_path.c_str()),
            const_cast<char*>(arg_fd.c_str()),
            nullptr,
        };
        ::execv(host_path.c_str(), argv);
        ::_exit(127);
    }

    ::close(sv[1]);
    fd_ = sv[0];
    child_pid_ = static_cast<int>(pid);
    return true;
}

bool AxPluginProcess::SendInit(std::string* error) {
    if (fd_ < 0) {
        if (error) *error = "plugin process not spawned";
        return false;
    }

    ipc::InitReq req{};
    req.device_id = device_id_;
    req.so_path_len = static_cast<std::uint32_t>(so_path_.size());
    req.init_json_len = static_cast<std::uint32_t>(init_json_.size());

    const std::size_t payload_size = sizeof(req) + so_path_.size() + init_json_.size();
    ipc::MsgHeader h{};
    h.magic = ipc::kMagic;
    h.version = ipc::kVersion;
    h.type = static_cast<std::uint16_t>(ipc::MsgType::kInitReq);
    h.payload_size = static_cast<std::uint32_t>(payload_size);

    std::vector<std::uint8_t> payload;
    payload.resize(payload_size);
    std::size_t off = 0;
    std::memcpy(payload.data() + off, &req, sizeof(req));
    off += sizeof(req);
    if (!so_path_.empty()) {
        std::memcpy(payload.data() + off, so_path_.data(), so_path_.size());
        off += so_path_.size();
    }
    if (!init_json_.empty()) {
        std::memcpy(payload.data() + off, init_json_.data(), init_json_.size());
        off += init_json_.size();
    }

    if (!WriteExact(fd_, &h, sizeof(h)) || !WriteExact(fd_, payload.data(), payload.size())) {
        if (error) *error = "write init request failed";
        return false;
    }

    ipc::MsgHeader rh{};
    if (!ReadExact(fd_, &rh, sizeof(rh))) {
        if (error) *error = "read init response header failed";
        return false;
    }
    if (rh.magic != ipc::kMagic || rh.version != ipc::kVersion ||
        rh.type != static_cast<std::uint16_t>(ipc::MsgType::kInitResp) ||
        rh.payload_size < sizeof(ipc::InitResp)) {
        if (error) *error = "invalid init response header";
        return false;
    }

    std::vector<std::uint8_t> rpayload;
    rpayload.resize(rh.payload_size);
    if (!ReadExact(fd_, rpayload.data(), rpayload.size())) {
        if (error) *error = "read init response payload failed";
        return false;
    }

    ipc::InitResp resp{};
    std::memcpy(&resp, rpayload.data(), sizeof(resp));
    if (resp.status == 0) {
        return true;
    }

    std::string perr;
    if (resp.err_len > 0 && sizeof(resp) + resp.err_len <= rpayload.size()) {
        perr.assign(reinterpret_cast<const char*>(rpayload.data() + sizeof(resp)),
                    reinterpret_cast<const char*>(rpayload.data() + sizeof(resp) + resp.err_len));
    } else {
        perr = "plugin init failed";
    }
    if (error) *error = perr;
    return false;
}

bool AxPluginProcess::EnsureRunning(std::string* error) {
    if (fd_ >= 0 && child_pid_ > 0) {
        int status = 0;
        const pid_t r = ::waitpid(static_cast<pid_t>(child_pid_), &status, WNOHANG);
        if (r == 0) {
            return true;  // still running
        }
        // child exited/crashed
        ::close(fd_);
        fd_ = -1;
        child_pid_ = -1;
    }

    // Backoff to avoid hot restart loop.
    const std::uint64_t now = NowMs();
    const std::uint64_t since = (last_restart_ms_ == 0) ? 0 : (now - last_restart_ms_);
    const std::uint64_t delay_ms = std::min<std::uint64_t>(5000, 200 * (restart_failures_ + 1));
    if (since < delay_ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms - since));
    }
    last_restart_ms_ = NowMs();

    std::string err;
    if (!Spawn(&err) || !SendInit(&err)) {
        ++restart_failures_;
        if (error) *error = err.empty() ? "restart plugin process failed" : err;
        return false;
    }
    restart_failures_ = 0;
    return true;
}

bool AxPluginProcess::SendInferRequest(const axvsdk::common::AxImage& image,
                                       std::vector<axpipeline::ai::Detection>* out,
                                       std::string* error) {
    if (fd_ < 0) {
        if (error) *error = "plugin process not running";
        return false;
    }

    std::vector<std::uint8_t> payload;
    std::string ser_err;
    if (!SerializeImageToHostPayload(image, device_id_, &payload, &ser_err)) {
        if (error) *error = ser_err.empty() ? "serialize image failed" : ser_err;
        return false;
    }

    ipc::MsgHeader h{};
    h.magic = ipc::kMagic;
    h.version = ipc::kVersion;
    h.type = static_cast<std::uint16_t>(ipc::MsgType::kInferReq);
    h.payload_size = static_cast<std::uint32_t>(payload.size());

    if (!WriteExact(fd_, &h, sizeof(h)) || !WriteExact(fd_, payload.data(), payload.size())) {
        if (error) *error = "write infer request failed";
        return false;
    }

    ipc::MsgHeader rh{};
    if (!ReadExact(fd_, &rh, sizeof(rh))) {
        if (error) *error = "read infer response header failed";
        return false;
    }
    if (rh.magic != ipc::kMagic || rh.version != ipc::kVersion ||
        rh.type != static_cast<std::uint16_t>(ipc::MsgType::kInferResp) ||
        rh.payload_size < sizeof(ipc::InferResp)) {
        if (error) *error = "invalid infer response header";
        return false;
    }

    std::vector<std::uint8_t> rpayload;
    rpayload.resize(rh.payload_size);
    if (!ReadExact(fd_, rpayload.data(), rpayload.size())) {
        if (error) *error = "read infer response payload failed";
        return false;
    }

    ipc::InferResp resp{};
    std::memcpy(&resp, rpayload.data(), sizeof(resp));
    if (resp.status != 0) {
        std::string perr;
        if (resp.err_len > 0 && sizeof(resp) + resp.err_len <= rpayload.size()) {
            perr.assign(reinterpret_cast<const char*>(rpayload.data() + sizeof(resp)),
                        reinterpret_cast<const char*>(rpayload.data() + sizeof(resp) + resp.err_len));
        } else {
            perr = "plugin infer failed";
        }
        if (error) *error = perr;
        return false;
    }

    if (out == nullptr) return true;
    out->clear();

    const std::size_t det_bytes = static_cast<std::size_t>(resp.det_count) * sizeof(ax_plugin_det_t);
    const std::size_t need = sizeof(resp) + det_bytes;
    if (need > rpayload.size()) {
        if (error) *error = "infer response truncated";
        return false;
    }

    const auto* dets = reinterpret_cast<const ax_plugin_det_t*>(rpayload.data() + sizeof(resp));
    out->reserve(resp.det_count);
    for (std::size_t i = 0; i < resp.det_count; ++i) {
        const auto& d = dets[i];
        axpipeline::ai::Detection dd{};
        dd.x0 = d.x0;
        dd.y0 = d.y0;
        dd.x1 = d.x1;
        dd.y1 = d.y1;
        dd.score = d.score;
        dd.class_id = static_cast<int>(d.class_id);
        out->push_back(dd);
    }
    return true;
}

#endif  // __linux__ || __unix__

bool AxPluginProcess::Infer(const axvsdk::common::AxImage& image,
                            std::vector<axpipeline::ai::Detection>* out,
                            std::string* error) {
#if !(defined(__linux__) || defined(__unix__))
    (void)image;
    if (out) out->clear();
    if (error) *error = "subprocess plugin is only supported on unix";
    return false;
#else
    if (out) out->clear();
    std::string err;
    if (!EnsureRunning(&err)) {
        if (error) *error = err;
        return false;
    }

    if (SendInferRequest(image, out, &err)) {
        return true;
    }

    // If the child crashed mid-request, attempt one restart and retry once.
    std::string restart_err;
    if (!EnsureRunning(&restart_err)) {
        if (error) *error = err;
        return false;
    }
    if (SendInferRequest(image, out, &restart_err)) {
        return true;
    }

    if (error) *error = restart_err.empty() ? err : restart_err;
    return false;
#endif
}

}  // namespace axpipeline::plugin
