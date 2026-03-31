#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "cmdline.hpp"

#include "codec/ax_jpeg_codec.h"
#include "common/ax_drawer.h"
#include "common/ax_system.h"

#include "npu/models/ax_model_det.hpp"

namespace {

bool FileExists(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}

std::string BasenameNoExt(const std::string& p) {
    const std::filesystem::path pp(p);
    return pp.stem().string();
}

std::string JoinPath(const std::string& dir, const std::string& name) {
    return (std::filesystem::path(dir) / name).string();
}

std::string ToLower(std::string s) {
    for (auto& ch : s) {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    }
    return s;
}

bool EndsWithLower(const std::string& s, const std::string& suffix_lower) {
    const auto sl = ToLower(s);
    if (suffix_lower.size() > sl.size()) return false;
    return sl.compare(sl.size() - suffix_lower.size(), suffix_lower.size(), suffix_lower) == 0;
}

bool ReadFileBytes(const std::string& path, std::vector<std::uint8_t>* out) {
    if (!out) return false;
    out->clear();
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    ifs.seekg(0, std::ios::end);
    const auto end = ifs.tellg();
    if (end <= 0) return false;
    out->resize(static_cast<std::size_t>(end));
    ifs.seekg(0, std::ios::beg);
    if (!ifs.read(reinterpret_cast<char*>(out->data()), static_cast<std::streamsize>(out->size()))) return false;
    return true;
}

void ClampRect(float* x0, float* y0, float* x1, float* y1, std::uint32_t w, std::uint32_t h) {
    if (!x0 || !y0 || !x1 || !y1) return;
    if (*x1 < *x0) std::swap(*x0, *x1);
    if (*y1 < *y0) std::swap(*y0, *y1);
    const float fw = static_cast<float>(w);
    const float fh = static_cast<float>(h);
    *x0 = std::max(0.0F, std::min(*x0, fw));
    *x1 = std::max(0.0F, std::min(*x1, fw));
    *y0 = std::max(0.0F, std::min(*y0, fh));
    *y1 = std::max(0.0F, std::min(*y1, fh));
}

std::vector<std::string> DefaultImages() {
    std::vector<std::string> imgs;
    const std::vector<std::string> candidates = {
        "tmp/dumps_ax650/axp_frame.jpg",
        "tmp/from_board_ax630c_20260324_raw/axp_frame.jpg",
    };
    for (const auto& c : candidates) {
        if (FileExists(c)) imgs.push_back(c);
    }
    return imgs;
}

bool WriteBmpBgr24(const std::string& path, const std::uint8_t* bgr, std::uint32_t w, std::uint32_t h) {
    if (!bgr || w == 0 || h == 0) return false;

    // 24-bit BMP stores pixels as BGR with rows padded to 4 bytes and default bottom-up layout.
    const std::uint32_t in_row = w * 3U;
    const std::uint32_t out_row = (in_row + 3U) & ~3U;
    const std::uint32_t pixel_bytes = out_row * h;

    // Pack to guarantee exact BMP header layout on all toolchains (including MSVC).
#pragma pack(push, 1)
    struct BmpFileHeader {
        std::uint16_t bfType;
        std::uint32_t bfSize;
        std::uint16_t bfReserved1;
        std::uint16_t bfReserved2;
        std::uint32_t bfOffBits;
    };
    struct BmpInfoHeader {
        std::uint32_t biSize;
        std::int32_t biWidth;
        std::int32_t biHeight;
        std::uint16_t biPlanes;
        std::uint16_t biBitCount;
        std::uint32_t biCompression;
        std::uint32_t biSizeImage;
        std::int32_t biXPelsPerMeter;
        std::int32_t biYPelsPerMeter;
        std::uint32_t biClrUsed;
        std::uint32_t biClrImportant;
    };
    struct BmpHeaderPacked {
        BmpFileHeader file;
        BmpInfoHeader info;
    };
#pragma pack(pop)

    BmpHeaderPacked hdr{};
    hdr.file.bfType = 0x4D42;  // 'BM'
    hdr.file.bfOffBits = static_cast<std::uint32_t>(sizeof(BmpHeaderPacked));
    hdr.file.bfSize = hdr.file.bfOffBits + pixel_bytes;
    hdr.file.bfReserved1 = 0;
    hdr.file.bfReserved2 = 0;

    hdr.info.biSize = 40;
    hdr.info.biWidth = static_cast<std::int32_t>(w);
    hdr.info.biHeight = static_cast<std::int32_t>(h);
    hdr.info.biPlanes = 1;
    hdr.info.biBitCount = 24;
    hdr.info.biCompression = 0;
    hdr.info.biSizeImage = pixel_bytes;
    hdr.info.biXPelsPerMeter = 0;
    hdr.info.biYPelsPerMeter = 0;
    hdr.info.biClrUsed = 0;
    hdr.info.biClrImportant = 0;

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;
    ofs.write(reinterpret_cast<const char*>(&hdr), static_cast<std::streamsize>(sizeof(hdr)));
    if (!ofs) return false;

    std::array<std::uint8_t, 3> pad{};
    const std::uint32_t pad_bytes = out_row - in_row;

    // Bottom-up: write last row first.
    for (std::uint32_t y = 0; y < h; ++y) {
        const std::uint32_t src_y = h - 1U - y;
        const auto* row = bgr + static_cast<std::size_t>(src_y) * static_cast<std::size_t>(in_row);
        ofs.write(reinterpret_cast<const char*>(row), static_cast<std::streamsize>(in_row));
        if (pad_bytes) {
            ofs.write(reinterpret_cast<const char*>(pad.data()), static_cast<std::streamsize>(pad_bytes));
        }
        if (!ofs) return false;
    }
    return true;
}

inline void PutPixelBgr(std::uint8_t* bgr, std::uint32_t w, std::uint32_t h, std::int32_t x, std::int32_t y,
                        std::uint8_t b, std::uint8_t g, std::uint8_t r) {
    if (!bgr) return;
    if (x < 0 || y < 0) return;
    if (static_cast<std::uint32_t>(x) >= w || static_cast<std::uint32_t>(y) >= h) return;
    const std::size_t idx = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                             static_cast<std::size_t>(x)) *
                            3U;
    bgr[idx + 0] = b;
    bgr[idx + 1] = g;
    bgr[idx + 2] = r;
}

void DrawRectBgr24(std::uint8_t* bgr, std::uint32_t w, std::uint32_t h, std::int32_t x, std::int32_t y,
                   std::uint32_t rw, std::uint32_t rh, std::uint32_t thickness, std::uint8_t b, std::uint8_t g,
                   std::uint8_t r) {
    if (!bgr || w == 0 || h == 0) return;
    if (rw == 0 || rh == 0) return;
    const std::int32_t x0 = x;
    const std::int32_t y0 = y;
    const std::int32_t x1 = x + static_cast<std::int32_t>(rw) - 1;
    const std::int32_t y1 = y + static_cast<std::int32_t>(rh) - 1;
    const std::uint32_t tmax = std::max<std::uint32_t>(1U, thickness);

    for (std::uint32_t t = 0; t < tmax; ++t) {
        const std::int32_t yt0 = y0 + static_cast<std::int32_t>(t);
        const std::int32_t yt1 = y1 - static_cast<std::int32_t>(t);
        for (std::int32_t xx = x0; xx <= x1; ++xx) {
            PutPixelBgr(bgr, w, h, xx, yt0, b, g, r);
            PutPixelBgr(bgr, w, h, xx, yt1, b, g, r);
        }
        const std::int32_t xt0 = x0 + static_cast<std::int32_t>(t);
        const std::int32_t xt1 = x1 - static_cast<std::int32_t>(t);
        for (std::int32_t yy = y0; yy <= y1; ++yy) {
            PutPixelBgr(bgr, w, h, xt0, yy, b, g, r);
            PutPixelBgr(bgr, w, h, xt1, yy, b, g, r);
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    cmdline::parser parser;
    parser.set_program_name("ax_pipeline_model_test");
    parser.add<std::string>("model", 'm', "axmodel path", true);
    parser.add<std::string>("type", 0, "model type: yolov5 | yolov8", false, "yolov8");
    parser.add<int>("device", 'd', "device id (AXCL: 0..N-1, MSP: keep -1)", false, -1);
    parser.add<std::string>("out_dir", 'o', "output dir for annotated jpg", false, "tmp/model_test_out");
    parser.add<float>("conf", 0, "confidence threshold for drawing", false, 0.25F);
    parser.add<int>("topk", 0, "print top-k detections", false, 10);
    parser.add("no_save", 0, "do not write annotated images");

    if (!parser.parse(argc, argv)) {
        std::cerr << parser.usage();
        return 1;
    }

    const std::string model_path = parser.get<std::string>("model");
    if (!FileExists(model_path)) {
        std::cerr << "model not found: " << model_path << "\n";
        return 2;
    }

    std::vector<std::string> images = parser.rest();
    if (images.empty()) {
        images = DefaultImages();
    }
    if (images.empty()) {
        std::cerr << "no images specified, and no default images found under tmp/\n";
        std::cerr << "usage: ax_pipeline_model_test -m <model.axmodel> [image1.jpg image2.jpg ...]\n";
        return 3;
    }

    const int device_id = parser.get<int>("device");
    const std::string out_dir = parser.get<std::string>("out_dir");
    const bool save = !parser.exist("no_save");

    if (save) {
        std::error_code ec;
        std::filesystem::create_directories(out_dir, ec);
    }

    axvsdk::common::SystemOptions sys{};
    sys.device_id = device_id;
    sys.enable_vdec = true;
    sys.enable_venc = true;
    sys.enable_ivps = true;
    if (!axvsdk::common::InitializeSystem(sys)) {
        std::cerr << "InitializeSystem failed\n";
        return 4;
    }
    struct SysGuard {
        ~SysGuard() { axvsdk::common::ShutdownSystem(); }
    } guard{};

    axpipeline::npu::YoloDetOptions opt{};
    opt.base.model_path = model_path;
    opt.base.device_id = device_id;
    opt.base.resize_mode = axvsdk::common::ResizeMode::kKeepAspectRatio;
    opt.base.h_align = axvsdk::common::ResizeAlign::kCenter;
    opt.base.v_align = axvsdk::common::ResizeAlign::kCenter;
    opt.base.background_color = 0;  // black
    opt.num_classes = 80;
    opt.conf_threshold = std::max(0.0F, parser.get<float>("conf"));
    opt.nms_threshold = 0.45F;

    const std::string type = parser.get<std::string>("type");
    std::unique_ptr<axpipeline::npu::AxModelBase> model;
    std::string err;
    if (type == "yolov5" || type == "YOLOV5") {
        auto m = std::make_unique<axpipeline::npu::AxModelYoloV5>();
        if (!m->Init(opt, &err)) {
            std::cerr << "model init failed: " << err << "\n";
            return 5;
        }
        model = std::move(m);
    } else if (type == "yolov8" || type == "YOLOV8") {
        auto m = std::make_unique<axpipeline::npu::AxModelYoloV8Native>();
        if (!m->Init(opt, &err)) {
            std::cerr << "model init failed: " << err << "\n";
            return 5;
        }
        model = std::move(m);
    } else {
        std::cerr << "unsupported --type: " << type << " (expect yolov5 or yolov8)\n";
        return 6;
    }

    std::cout << "[model] input w=" << model->input_spec().width
              << " h=" << model->input_spec().height
              << " fmt=" << static_cast<int>(model->input_spec().format) << "\n";

    auto drawer = axvsdk::common::CreateDrawer();
    if (!drawer) {
        std::cerr << "CreateDrawer failed\n";
        return 7;
    }

    const int topk = std::max(0, parser.get<int>("topk"));

    int failed = 0;
    for (const auto& img_path : images) {
        if (!FileExists(img_path)) {
            std::cerr << "[skip] image not found: " << img_path << "\n";
            continue;
        }

        axvsdk::common::AxImage::Ptr img;
        std::shared_ptr<std::vector<std::uint8_t>> ext_bgr;  // non-null only for external .bgr inputs
        if (EndsWithLower(img_path, ".bgr")) {
            const auto& in = model->input_spec();
            if (in.format != axvsdk::common::PixelFormat::kBgr24) {
                std::cerr << "[fail] .bgr input requires model input fmt=BGR24, got fmt="
                          << static_cast<int>(in.format) << "\n";
                failed++;
                continue;
            }
            std::vector<std::uint8_t> bytes;
            if (!ReadFileBytes(img_path, &bytes)) {
                std::cerr << "[fail] read .bgr failed: " << img_path << "\n";
                failed++;
                continue;
            }
            const std::size_t need = static_cast<std::size_t>(in.width) * static_cast<std::size_t>(in.height) * 3U;
            if (bytes.size() != need) {
                std::cerr << "[fail] .bgr size mismatch: " << img_path
                          << " bytes=" << bytes.size() << " need=" << need << "\n";
                failed++;
                continue;
            }

            auto buf = std::make_shared<std::vector<std::uint8_t>>(std::move(bytes));
            ext_bgr = buf;

            axvsdk::common::ImageDescriptor desc{};
            desc.format = axvsdk::common::PixelFormat::kBgr24;
            desc.width = in.width;
            desc.height = in.height;
            desc.strides[0] = static_cast<std::size_t>(in.width) * 3U;

            std::array<axvsdk::common::ExternalImagePlane, axvsdk::common::kMaxImagePlanes> planes{};
            planes[0].virtual_address = buf->data();
            planes[0].physical_address = 0;

            img = axvsdk::common::AxImage::WrapExternal(desc, planes, buf);
            if (!img) {
                std::cerr << "[fail] wrap .bgr as AxImage failed: " << img_path << "\n";
                failed++;
                continue;
            }
        } else {
            img = axvsdk::codec::DecodeJpegFile(img_path);
            if (!img) {
                std::cerr << "[fail] DecodeJpegFile failed: " << img_path << "\n";
                failed++;
                continue;
            }
        }

        std::vector<axpipeline::npu::Detection> dets;
        axpipeline::npu::RunTimings tm{};
        if (!model->Infer(*img, &dets, &err, &tm)) {
            std::cerr << "[fail] Infer failed: " << img_path << " err=" << err << "\n";
            failed++;
            continue;
        }

        std::sort(dets.begin(), dets.end(), [](const auto& a, const auto& b) { return a.score > b.score; });

        std::cout << "[image] " << img_path
                  << " w=" << img->width()
                  << " h=" << img->height()
                  << " dets=" << dets.size()
                  << " time_us={pre=" << tm.preprocess_us
                  << " infer=" << tm.infer_us
                  << " post=" << tm.postprocess_us
                  << " total=" << tm.total_us
                  << "}\n";

        const std::size_t limit = std::min<std::size_t>(static_cast<std::size_t>(topk), dets.size());
        for (std::size_t i = 0; i < limit; ++i) {
            const auto& d = dets[i];
            std::cout << "  det" << i
                      << " cls=" << d.class_id
                      << " score=" << d.score
                      << " box=(" << d.x0 << "," << d.y0 << ")-(" << d.x1 << "," << d.y1 << ")\n";
        }

        if (save) {
            if (ext_bgr) {
                // Save a viewable image without relying on JPEG encode (AXCL JPEG encode/decode is flaky).
                const auto iw = img->width();
                const auto ih = img->height();
                for (const auto& d : dets) {
                    if (d.score < opt.conf_threshold) continue;
                    float x0 = d.x0;
                    float y0 = d.y0;
                    float x1 = d.x1;
                    float y1 = d.y1;
                    ClampRect(&x0, &y0, &x1, &y1, iw, ih);
                    const auto rw = static_cast<std::int32_t>(x1 - x0);
                    const auto rh = static_cast<std::int32_t>(y1 - y0);
                    if (rw <= 2 || rh <= 2) continue;
                    DrawRectBgr24(ext_bgr->data(), iw, ih, static_cast<std::int32_t>(x0), static_cast<std::int32_t>(y0),
                                  static_cast<std::uint32_t>(rw), static_cast<std::uint32_t>(rh), 2, 0, 255, 0);
                }

                const std::string out_path = JoinPath(out_dir, BasenameNoExt(img_path) + "_det.bmp");
                if (!WriteBmpBgr24(out_path, ext_bgr->data(), img->width(), img->height())) {
                    std::cerr << "[warn] WriteBmpBgr24 failed: " << out_path << "\n";
                } else {
                    std::cout << "  saved: " << out_path << "\n";
                }
            } else {
                axvsdk::common::DrawFrame osd{};
                osd.hold_frames = 1;
                osd.rects.reserve(dets.size());
                for (const auto& d : dets) {
                    if (d.score < opt.conf_threshold) continue;
                    float x0 = d.x0;
                    float y0 = d.y0;
                    float x1 = d.x1;
                    float y1 = d.y1;
                    ClampRect(&x0, &y0, &x1, &y1, img->width(), img->height());
                    const auto rw = static_cast<std::int32_t>(x1 - x0);
                    const auto rh = static_cast<std::int32_t>(y1 - y0);
                    if (rw <= 2 || rh <= 2) continue;

                    axvsdk::common::DrawRect r{};
                    r.x = static_cast<std::int32_t>(x0);
                    r.y = static_cast<std::int32_t>(y0);
                    r.width = static_cast<std::uint32_t>(rw);
                    r.height = static_cast<std::uint32_t>(rh);
                    r.thickness = 2;
                    r.alpha = 255;
                    r.color = 0x00FF00;
                    osd.rects.push_back(r);
                }
                if (!osd.rects.empty()) {
                    (void)drawer->Draw(osd, *img);
                }

                const std::string out_path = JoinPath(out_dir, BasenameNoExt(img_path) + "_det.jpg");
                if (!axvsdk::codec::EncodeJpegToFile(*img, out_path)) {
                    std::cerr << "[warn] EncodeJpegToFile failed: " << out_path << "\n";
                } else {
                    std::cout << "  saved: " << out_path << "\n";
                }
            }
        }
    }

    if (failed) {
        std::cerr << "FAILED: " << failed << " images\n";
        return 10;
    }
    return 0;
}
