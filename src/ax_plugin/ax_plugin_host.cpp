#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "ax_plugin/ax_plugin.h"
#include "ax_plugin/ax_plugin_ipc.hpp"

#if defined(_WIN32)
int main() {
    return 1;
}
#else

#  include <dlfcn.h>
#  include <errno.h>
#  include <signal.h>
#  include <unistd.h>

namespace {

bool ReadExact(int fd, void* buf, std::size_t n) {
    std::uint8_t* p = static_cast<std::uint8_t*>(buf);
    std::size_t left = n;
    while (left > 0) {
        const ssize_t r = ::read(fd, p, left);
        if (r == 0) return false;
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

struct PluginFns {
    void* lib{nullptr};
    ax_plugin_handle_t handle{nullptr};

    int (*get_api_version)() = nullptr;
    int (*init)(const char*, std::int32_t, ax_plugin_handle_t*) = nullptr;
    void (*deinit)(ax_plugin_handle_t) = nullptr;
    int (*infer)(ax_plugin_handle_t, const ax_plugin_image_view_t*, ax_plugin_det_result_t*) = nullptr;
    void (*release)(ax_plugin_handle_t, ax_plugin_det_result_t*) = nullptr;

    void Reset() {
        if (handle && deinit) {
            try {
                deinit(handle);
            } catch (...) {
            }
        }
        handle = nullptr;
        get_api_version = nullptr;
        init = nullptr;
        deinit = nullptr;
        infer = nullptr;
        release = nullptr;
        if (lib) {
            ::dlclose(lib);
        }
        lib = nullptr;
    }
};

bool LoadPlugin(const std::string& so_path,
                const std::string& init_json,
                std::int32_t device_id,
                PluginFns* f,
                std::string* error) {
    if (f == nullptr) return false;
    f->Reset();

    void* lib = ::dlopen(so_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        if (error) *error = std::string("dlopen failed: ") + (::dlerror() ? ::dlerror() : "");
        return false;
    }

    auto load_sym = [&](const char* name) -> void* { return ::dlsym(lib, name); };
    f->get_api_version = reinterpret_cast<int (*)()>(load_sym("ax_plugin_get_api_version"));
    f->init = reinterpret_cast<int (*)(const char*, std::int32_t, ax_plugin_handle_t*)>(load_sym("ax_plugin_init"));
    f->deinit = reinterpret_cast<void (*)(ax_plugin_handle_t)>(load_sym("ax_plugin_deinit"));
    f->infer = reinterpret_cast<int (*)(ax_plugin_handle_t, const ax_plugin_image_view_t*, ax_plugin_det_result_t*)>(
        load_sym("ax_plugin_infer"));
    f->release = reinterpret_cast<void (*)(ax_plugin_handle_t, ax_plugin_det_result_t*)>(load_sym("ax_plugin_release_result"));

    if (!f->get_api_version || !f->init || !f->deinit || !f->infer || !f->release) {
        ::dlclose(lib);
        if (error) *error = "plugin missing required symbols";
        return false;
    }

    int api = 0;
    try {
        api = f->get_api_version();
    } catch (const std::exception& e) {
        ::dlclose(lib);
        if (error) *error = std::string("plugin get_api_version threw: ") + e.what();
        return false;
    } catch (...) {
        ::dlclose(lib);
        if (error) *error = "plugin get_api_version threw";
        return false;
    }
    if (api != AX_PLUGIN_API_VERSION) {
        ::dlclose(lib);
        if (error) *error = "plugin api version mismatch";
        return false;
    }

    ax_plugin_handle_t h = nullptr;
    int ret = 0;
    try {
        ret = f->init(init_json.c_str(), device_id, &h);
    } catch (const std::exception& e) {
        ::dlclose(lib);
        if (error) *error = std::string("plugin init threw: ") + e.what();
        return false;
    } catch (...) {
        ::dlclose(lib);
        if (error) *error = "plugin init threw";
        return false;
    }

    if (ret != 0 || h == nullptr) {
        ::dlclose(lib);
        if (error) *error = "plugin init failed: ret=" + std::to_string(ret);
        return false;
    }

    f->lib = lib;
    f->handle = h;
    return true;
}

int ParseIpcFd(int argc, char** argv) {
    const std::string key = "--ipc-fd=";
    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) continue;
        std::string s(argv[i]);
        if (s.rfind(key, 0) == 0) {
            try {
                return std::stoi(s.substr(key.size()));
            } catch (...) {
                return -1;
            }
        }
    }
    return -1;
}

}  // namespace

int main(int argc, char** argv) {
    const int fd = ParseIpcFd(argc, argv);
    if (fd < 0) return 2;

    ::signal(SIGPIPE, SIG_IGN);

    PluginFns plugin;
    std::vector<std::uint8_t> payload;
    payload.reserve(4 * 1024 * 1024);

    for (;;) {
        axpipeline::plugin::ipc::MsgHeader h{};
        if (!ReadExact(fd, &h, sizeof(h))) {
            break;
        }
        if (h.magic != axpipeline::plugin::ipc::kMagic || h.version != axpipeline::plugin::ipc::kVersion) {
            break;
        }

        payload.resize(h.payload_size);
        if (h.payload_size > 0) {
            if (!ReadExact(fd, payload.data(), payload.size())) {
                break;
            }
        }

        const auto type = static_cast<axpipeline::plugin::ipc::MsgType>(h.type);
        if (type == axpipeline::plugin::ipc::MsgType::kInitReq) {
            std::string err;
            int status = 0;
            if (payload.size() < sizeof(axpipeline::plugin::ipc::InitReq)) {
                status = -1;
                err = "bad init payload";
            } else {
                axpipeline::plugin::ipc::InitReq req{};
                std::memcpy(&req, payload.data(), sizeof(req));
                const std::size_t need = sizeof(req) + req.so_path_len + req.init_json_len;
                if (need > payload.size()) {
                    status = -2;
                    err = "bad init payload size";
                } else {
                    std::string so_path;
                    so_path.assign(reinterpret_cast<const char*>(payload.data() + sizeof(req)), req.so_path_len);
                    std::string init_json;
                    init_json.assign(reinterpret_cast<const char*>(payload.data() + sizeof(req) + req.so_path_len),
                                     req.init_json_len);

                    std::string load_err;
                    if (!LoadPlugin(so_path, init_json, req.device_id, &plugin, &load_err)) {
                        status = -3;
                        err = load_err.empty() ? "LoadPlugin failed" : load_err;
                    }
                }
            }

            axpipeline::plugin::ipc::InitResp resp{};
            resp.status = status;
            resp.err_len = static_cast<std::uint32_t>(err.size());

            axpipeline::plugin::ipc::MsgHeader rh{};
            rh.magic = axpipeline::plugin::ipc::kMagic;
            rh.version = axpipeline::plugin::ipc::kVersion;
            rh.type = static_cast<std::uint16_t>(axpipeline::plugin::ipc::MsgType::kInitResp);
            rh.payload_size = static_cast<std::uint32_t>(sizeof(resp) + err.size());

            if (!WriteExact(fd, &rh, sizeof(rh)) || !WriteExact(fd, &resp, sizeof(resp)) ||
                (!err.empty() && !WriteExact(fd, err.data(), err.size()))) {
                break;
            }
            continue;
        }

        if (type == axpipeline::plugin::ipc::MsgType::kInferReq) {
            std::string err;
            int status = 0;
            std::vector<ax_plugin_det_t> dets_copy;

            if (!plugin.handle || !plugin.infer || !plugin.release) {
                status = -1;
                err = "plugin not initialized";
            } else if (payload.size() < sizeof(axpipeline::plugin::ipc::InferReq)) {
                status = -2;
                err = "bad infer payload";
            } else {
                axpipeline::plugin::ipc::InferReq req{};
                std::memcpy(&req, payload.data(), sizeof(req));
                const std::size_t pc = req.plane_count > 3 ? 3 : req.plane_count;

                std::size_t need = sizeof(req);
                for (std::size_t i = 0; i < pc; ++i) {
                    need += static_cast<std::size_t>(req.plane_sizes[i]);
                }
                if (need > payload.size()) {
                    status = -3;
                    err = "bad infer payload size";
                } else {
                    ax_plugin_image_view_t view{};
                    view.format = static_cast<ax_plugin_pixel_format_e>(req.format);
                    view.width = req.width;
                    view.height = req.height;
                    view.plane_count = pc;
                    view.memory_type = AX_PLUGIN_MEMORY_TYPE_HOST;
                    for (std::size_t i = 0; i < 3; ++i) {
                        view.strides[i] = 0;
                        view.physical_addrs[i] = 0;
                        view.virtual_addrs[i] = nullptr;
                        view.block_ids[i] = 0xFFFFFFFFU;
                    }

                    std::size_t off = sizeof(req);
                    for (std::size_t i = 0; i < pc; ++i) {
                        view.strides[i] = static_cast<std::size_t>(req.strides[i]);
                        const std::size_t psz = static_cast<std::size_t>(req.plane_sizes[i]);
                        view.virtual_addrs[i] = payload.data() + off;
                        off += psz;
                    }

                    ax_plugin_det_result_t result{};
                    int ret = 0;
                    try {
                        ret = plugin.infer(plugin.handle, &view, &result);
                    } catch (const std::exception& e) {
                        ret = -999;
                        err = std::string("plugin infer threw: ") + e.what();
                    } catch (...) {
                        ret = -999;
                        err = "plugin infer threw";
                    }

                    if (ret != 0) {
                        status = ret;
                        if (err.empty()) err = "plugin infer failed: ret=" + std::to_string(ret);
                    } else {
                        if (result.dets && result.det_count > 0) {
                            dets_copy.assign(result.dets, result.dets + result.det_count);
                        }
                        try {
                            plugin.release(plugin.handle, &result);
                        } catch (...) {
                        }
                    }
                }
            }

            axpipeline::plugin::ipc::InferResp resp{};
            resp.status = status;
            resp.det_count = static_cast<std::uint32_t>(dets_copy.size());
            resp.err_len = static_cast<std::uint32_t>((status == 0) ? 0U : err.size());

            const std::size_t det_bytes = dets_copy.size() * sizeof(ax_plugin_det_t);
            const std::size_t payload_size = sizeof(resp) + det_bytes + ((status == 0) ? 0U : err.size());

            axpipeline::plugin::ipc::MsgHeader rh{};
            rh.magic = axpipeline::plugin::ipc::kMagic;
            rh.version = axpipeline::plugin::ipc::kVersion;
            rh.type = static_cast<std::uint16_t>(axpipeline::plugin::ipc::MsgType::kInferResp);
            rh.payload_size = static_cast<std::uint32_t>(payload_size);

            if (!WriteExact(fd, &rh, sizeof(rh))) break;
            if (!WriteExact(fd, &resp, sizeof(resp))) break;
            if (det_bytes > 0 && !WriteExact(fd, dets_copy.data(), det_bytes)) break;
            if (status != 0 && !err.empty() && !WriteExact(fd, err.data(), err.size())) break;
            continue;
        }

        if (type == axpipeline::plugin::ipc::MsgType::kShutdownReq) {
            axpipeline::plugin::ipc::ShutdownResp resp{};
            resp.status = 0;
            axpipeline::plugin::ipc::MsgHeader rh{};
            rh.magic = axpipeline::plugin::ipc::kMagic;
            rh.version = axpipeline::plugin::ipc::kVersion;
            rh.type = static_cast<std::uint16_t>(axpipeline::plugin::ipc::MsgType::kShutdownResp);
            rh.payload_size = sizeof(resp);
            (void)WriteExact(fd, &rh, sizeof(rh));
            (void)WriteExact(fd, &resp, sizeof(resp));
            break;
        }

        // Unknown message type.
        break;
    }

    plugin.Reset();
    return 0;
}

#endif  // !_WIN32

