#include "ax_version_check.h"

#include <string>
#include <vector>
#include "sample_log.h"

#define AX_BOARD_VERSION_PATH "/proc/ax_proc/version"

static std::vector<std::string> split_str(const std::string &str, const std::string &delim)
{
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos)
            pos = str.length();
        std::string token = str.substr(prev, pos - prev);
        if (!token.empty())
            tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}

static std::string trim_str(const std::string &str, const std::string &delim = " ")
{
    std::string s = str;
    s.erase(0, s.find_first_not_of(delim));
    s.erase(s.find_last_not_of(delim) + 1);
    return s;
}

static int get_version(std::string version_str, std::string &major_version, std::string &minor_version, std::string &build_time)
{
    auto tokens = split_str(version_str, "_");
    if (tokens.size() != 3)
    {
        ALOGW("invalid version string: %s", version_str.c_str());
        major_version = tokens[0];
        return 0;
    }

    // trim " "
    major_version = trim_str(tokens[0]);
    minor_version = trim_str(tokens[1]);
    build_time = trim_str(tokens[2]);

    // trim "\n"
    major_version = trim_str(major_version, "\n");
    minor_version = trim_str(minor_version, "\n");
    build_time = trim_str(build_time, "\n");

    return 0;
}

// for example: "Ax_Version V1.45.0_P1_20230922094955"
// return V1.45.0
static int get_board_version(std::string &major_version, std::string &minor_version, std::string &build_time)
{
    FILE *fp = fopen(AX_BOARD_VERSION_PATH, "r");
    if (fp == NULL)
    {
        ALOGE("open %s failed", AX_BOARD_VERSION_PATH);
        return -1;
    }
    char buf[256] = {0};
    fgets(buf, sizeof(buf), fp);
    fclose(fp);
    std::string version = std::string(buf);
    std::vector<std::string> tokens = split_str(version, " ");
    if (tokens.size() != 2)
    {
        ALOGE("invalid board version: %s", version.c_str());
        return -1;
    }

    return get_version(tokens[1], major_version, minor_version, build_time);
}

int ax_version_check()
{
    std::string board_major_version;
    std::string board_minor_version;
    std::string board_build_time;
    int ret = get_board_version(board_major_version, board_minor_version, board_build_time);
    if (ret != 0)
    {
        ALOGE("get board version failed");
        return -1;
    }

    std::string compile_major_version;
    std::string compile_minor_version;
    std::string compile_build_time;
    // AXERA_BSP_VERSION define by cmake
    ret = get_version(AXERA_BSP_VERSION, compile_major_version, compile_minor_version, compile_build_time);
    if (ret != 0)
    {
        ALOGE("get compile version failed");
        return -1;
    }

    ALOGI("\n\n     >>>>   board version: [%s] [%s] [%s] <<<<\n     >>>> compile version: [%s] [%s] [%s] <<<<\n",
          board_major_version.c_str(), board_minor_version.c_str(), board_build_time.c_str(),
          compile_major_version.c_str(), compile_minor_version.c_str(), compile_build_time.c_str());

    if (board_major_version != compile_major_version)
    {
        ALOGE("major version not match: [%s] != [%s]", board_major_version.c_str(), compile_major_version.c_str());
        return -1;
    }

    if (board_minor_version != compile_minor_version)
    {
        ALOGW("minor version not match: [%s] != [%s]", board_minor_version.c_str(), compile_minor_version.c_str());
    }

    if (board_build_time != compile_build_time)
    {
        ALOGW("build time not match: [%s] != [%s]", board_build_time.c_str(), compile_build_time.c_str());
    }

    return 0;
}