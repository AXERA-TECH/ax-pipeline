#pragma once

#include <cstddef>
#include <cstdint>

namespace axpipeline::plugin::ipc {

constexpr std::uint32_t kMagic = 0x504C5841U;  // 'AXLP'
constexpr std::uint16_t kVersion = 1;

enum class MsgType : std::uint16_t {
    kInitReq = 1,
    kInitResp = 2,
    kInferReq = 3,
    kInferResp = 4,
    kShutdownReq = 5,
    kShutdownResp = 6,
};

struct MsgHeader {
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t type;
    std::uint32_t payload_size;
};

struct InitReq {
    std::int32_t device_id;
    std::uint32_t so_path_len;
    std::uint32_t init_json_len;
};

struct InitResp {
    std::int32_t status;  // 0 ok, non-zero error
    std::uint32_t err_len;
};

struct InferReq {
    std::uint32_t format;  // ax_plugin_pixel_format_e
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t plane_count;
    std::uint64_t strides[3];
    std::uint64_t plane_sizes[3];
    std::uint32_t reserved;
};

struct InferResp {
    std::int32_t status;  // 0 ok, non-zero error
    std::uint32_t det_count;
    std::uint32_t err_len;
};

struct ShutdownReq {
    std::uint32_t reserved;
};

struct ShutdownResp {
    std::int32_t status;
};

static_assert(sizeof(MsgHeader) == 12, "MsgHeader size");
static_assert(sizeof(InitReq) == 12, "InitReq size");
static_assert(sizeof(InitResp) == 8, "InitResp size");
static_assert(sizeof(InferReq) == 72, "InferReq size");
static_assert(sizeof(InferResp) == 12, "InferResp size");

}  // namespace axpipeline::plugin::ipc

