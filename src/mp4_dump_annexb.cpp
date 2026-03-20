#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "cmdline.hpp"

// Internal header from the ax-video-sdk submodule.
// This tool is for debugging/validation and is not part of the public SDK surface.
#include "ax_mp4_demuxer.h"

namespace {

bool LooksLikeAnnexBPrefix(const std::uint8_t* p, std::size_t n) {
    if (p == nullptr || n < 3) return false;
    if (n >= 4 && p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01) return true;
    if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01) return true;
    return false;
}

void PrintPrefix(const std::string& label, const std::uint8_t* p, std::size_t n) {
    std::cerr << label << ": ";
    const auto k = (n < 16) ? n : 16;
    for (std::size_t i = 0; i < k; ++i) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(p[i]);
    }
    std::cerr << std::dec << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    cmdline::parser parser;
    parser.set_program_name("ax_mp4_dump_annexb");
    parser.add<std::string>("input", 'i', "input MP4 file path", true);
    parser.add<std::string>("output", 'o', "output elementary stream path (.h264/.h265)", true);
    parser.add<int>("max", 'n', "max packets to dump (0 = dump all)", false, 0);
    parser.add("verbose", 0, "print per-packet info (first/last 3 packets)");

    if (!parser.parse(argc, argv)) {
        std::cerr << parser.error() << "\n" << parser.usage();
        return 1;
    }

    const auto input = parser.get<std::string>("input");
    const auto output = parser.get<std::string>("output");
    const int max_packets = parser.get<int>("max");
    const bool verbose = parser.exist("verbose");

    auto demuxer = axvsdk::codec::AxMp4Demuxer::Open(input);
    if (!demuxer) {
        std::cerr << "Open mp4 failed: " << input << "\n";
        return 2;
    }

    std::ofstream out(output, std::ios::binary);
    if (!out) {
        std::cerr << "Open output failed: " << output << "\n";
        return 3;
    }

    std::uint64_t total_bytes = 0;
    int packets = 0;
    bool saw_annexb_prefix = false;
    bool saw_non_annexb_prefix = false;

    axvsdk::codec::EncodedPacket pkt;
    while (max_packets <= 0 || packets < max_packets) {
        pkt = {};
        if (!demuxer->ReadNextPacket(&pkt)) break;
        if (!pkt.data.empty()) {
            const bool annexb = LooksLikeAnnexBPrefix(pkt.data.data(), pkt.data.size());
            saw_annexb_prefix |= annexb;
            saw_non_annexb_prefix |= !annexb;
            if (packets == 0) {
                PrintPrefix("first_packet_prefix", pkt.data.data(), pkt.data.size());
            }
        }

        if (verbose && (packets < 3)) {
            std::cerr << "pkt[" << packets << "] pts=" << pkt.pts
                      << " dur=" << pkt.duration
                      << " bytes=" << pkt.data.size()
                      << " key=" << (pkt.key_frame ? 1 : 0) << "\n";
        }

        if (!pkt.data.empty()) {
            out.write(reinterpret_cast<const char*>(pkt.data.data()),
                      static_cast<std::streamsize>(pkt.data.size()));
            total_bytes += pkt.data.size();
        }
        ++packets;
    }

    out.flush();
    std::cerr << "dump_done: packets=" << packets
              << " total_bytes=" << total_bytes
              << " annexb_prefix_seen=" << (saw_annexb_prefix ? 1 : 0)
              << " non_annexb_prefix_seen=" << (saw_non_annexb_prefix ? 1 : 0)
              << "\n";
    return 0;
}

