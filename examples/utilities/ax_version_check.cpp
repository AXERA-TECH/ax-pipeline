#include "ax_version_check.h"

#include <string>
#include <vector>
#include <fstream>

#include "sample_log.h"

#define AX_BOARD_VERSION_PATH "/proc/ax_proc/version"

static std::vector<std::string> v_libax_sys_so_path = {
    "/soc/lib/libax_sys.so",
    "/opt/lib/libax_sys.so",
};

static std::string exec_cmd(std::string cmd)
{
    ALOGI("exec cmd: %s", cmd.c_str());
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        return "";
    }
    char buffer[128];
    std::string result = "";
    while (!feof(pipe))
    {
        if (fgets(buffer, 128, pipe) != NULL)
        {
            result += buffer;
        }
    }
    pclose(pipe);
    return result;
}

static bool file_exists(const std::string &name)
{
    std::ifstream f(name.c_str());
    return f.good();
}

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
    if (tokens.size() == 2)
    {
        // trim " "
        major_version = trim_str(tokens[0]);
        minor_version = "";
        build_time = trim_str(tokens[1]);

        // trim "\n"
        major_version = trim_str(major_version, "\n");
        minor_version = trim_str(minor_version, "\n");
        build_time = trim_str(build_time, "\n");

        return 0;
    }
    else if (tokens.size() == 3)
    {
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
    ALOGE("invalid version string: %s", version_str.c_str());
    return -1;
}

// for example: "Ax_Version V1.45.0_P1_20230922094955"
// return V1.45.0
static int get_board_version(std::string &major_version, std::string &minor_version, std::string &build_time)
{
    std::string version = exec_cmd("cat " + std::string(AX_BOARD_VERSION_PATH) + " |awk '{print $2}'");
    if (version.empty())
    {
        ALOGE("get board version failed");
        return -1;
    }

    return get_version(version, major_version, minor_version, build_time);
}

static int get_board_so_version(std::string &major_version, std::string &minor_version, std::string &build_time)
{
    // std::string cmd = "strings ${BSP_MSP_DIR}/lib/libax_sys.so | grep 'Axera version' | awk '{print $4}'";
    char cmd[128] = {0};
    for (size_t i = 0; i < v_libax_sys_so_path.size(); i++)
    {
        if (!file_exists(v_libax_sys_so_path[i]))
        {
            continue;
        }
        sprintf(cmd, "strings %s | grep 'Axera version' | awk '{print $4}'", v_libax_sys_so_path[i].c_str());
        std::string version = exec_cmd(cmd);
        if (!version.empty())
        {
            if (get_version(version, major_version, minor_version, build_time) == 0)
            {
                return 0;
            }
        }
    }
    return -1;
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
        ALOGI("try to get board so version");
        ret = get_board_so_version(board_major_version, board_minor_version, board_build_time);
        if (ret != 0)
        {
            ALOGE("get board so version failed");
            return -1;
        }
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