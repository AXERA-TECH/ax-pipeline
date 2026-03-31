#pragma once

#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string.h>
#include <vector>

typedef struct {
    std::string sName;
    unsigned int nIdx;
    std::vector<unsigned int> vShape;
    int nSize;
    unsigned long phyAddr;
    void* pVirAddr;
} ax_runner_tensor_t;

class ax_runner_base {
protected:
    std::vector<ax_runner_tensor_t> moutput_tensors;
    std::vector<ax_runner_tensor_t> minput_tensors;

    std::vector<std::vector<ax_runner_tensor_t>> mgroup_output_tensors;
    std::vector<std::vector<ax_runner_tensor_t>> mgroup_input_tensors;

    std::map<std::string, ax_runner_tensor_t> map_output_tensors;
    std::map<std::string, ax_runner_tensor_t> map_input_tensors;

    std::map<std::string, std::vector<ax_runner_tensor_t>> map_group_output_tensors;
    std::map<std::string, std::vector<ax_runner_tensor_t>> map_group_input_tensors;

    int _devid = 0;

public:
    virtual int init(const void* model_data, unsigned int model_size, int devid) = 0;
    virtual void deinit() = 0;
    virtual int set_affinity(int id) { return -1; };

    int get_num_inputs() { return static_cast<int>(minput_tensors.size()); };
    int get_num_outputs() { return static_cast<int>(moutput_tensors.size()); };

    int get_num_input_groups() { return static_cast<int>(mgroup_input_tensors.size()); };
    int get_num_output_groups() { return static_cast<int>(mgroup_output_tensors.size()); };

    virtual int sync_input(int idx) = 0;
    virtual int sync_input(std::string name) = 0;
    virtual int sync_output(int idx) = 0;
    virtual int sync_output(std::string name) = 0;

    virtual int sync_input(int grpid, int idx) = 0;
    virtual int sync_input(int grpid, std::string name) = 0;
    virtual int sync_output(int grpid, int idx) = 0;
    virtual int sync_output(int grpid, std::string name) = 0;

    const ax_runner_tensor_t& get_input(int idx) { return minput_tensors[idx]; }
    const ax_runner_tensor_t* get_inputs_ptr() { return minput_tensors.data(); }
    const ax_runner_tensor_t& get_input(std::string name) {
        if (map_input_tensors.empty()) {
            for (size_t i = 0; i < minput_tensors.size(); i++) {
                map_input_tensors[minput_tensors[i].sName] = minput_tensors[i];
            }
        }
        if (map_input_tensors.find(name) == map_input_tensors.end()) {
            throw std::runtime_error("input tensor not found: " + name);
        }
        return map_input_tensors[name];
    }

    const ax_runner_tensor_t& get_input(int grpid, int idx) { return mgroup_input_tensors[grpid][idx]; }
    const ax_runner_tensor_t* get_inputs_ptr(int grpid) { return mgroup_input_tensors[grpid].data(); }
    const ax_runner_tensor_t& get_input(int grpid, std::string name) {
        if (map_group_input_tensors.empty()) {
            for (size_t i = 0; i < mgroup_input_tensors.size(); i++) {
                for (size_t j = 0; j < mgroup_input_tensors[i].size(); j++) {
                    map_group_input_tensors[mgroup_input_tensors[i][j].sName].push_back(mgroup_input_tensors[i][j]);
                }
            }
        }
        if (map_group_input_tensors.find(name) == map_group_input_tensors.end()) {
            throw std::runtime_error("input tensor not found: " + name);
        }
        return map_group_input_tensors[name][grpid];
    }

    const ax_runner_tensor_t& get_output(int idx) { return moutput_tensors[idx]; }
    const ax_runner_tensor_t* get_outputs_ptr() { return moutput_tensors.data(); }
    const ax_runner_tensor_t& get_output(std::string name) {
        if (map_output_tensors.empty()) {
            for (size_t i = 0; i < moutput_tensors.size(); i++) {
                map_output_tensors[moutput_tensors[i].sName] = moutput_tensors[i];
            }
        }
        if (map_output_tensors.find(name) == map_output_tensors.end()) {
            throw std::runtime_error("output tensor not found: " + name);
        }
        return map_output_tensors[name];
    }

    const ax_runner_tensor_t& get_output(int grpid, int idx) { return mgroup_output_tensors[grpid][idx]; }
    const ax_runner_tensor_t* get_outputs_ptr(int grpid) { return mgroup_output_tensors[grpid].data(); }
    const ax_runner_tensor_t& get_output(int grpid, std::string name) {
        if (map_group_output_tensors.empty()) {
            for (size_t i = 0; i < mgroup_output_tensors.size(); i++) {
                for (size_t j = 0; j < mgroup_output_tensors[i].size(); j++) {
                    map_group_output_tensors[mgroup_output_tensors[i][j].sName].push_back(mgroup_output_tensors[i][j]);
                }
            }
        }
        if (map_group_output_tensors.find(name) == map_group_output_tensors.end()) {
            throw std::runtime_error("input tensor not found: " + name);
        }
        return map_group_output_tensors[name][grpid];
    }

    virtual int get_algo_width() {
        if (minput_tensors.size() == 1 && minput_tensors[0].vShape.size() == 4) {
            return static_cast<int>(minput_tensors[0].vShape[2]);
        }
        return -1;
    }

    virtual int get_algo_height() {
        if (minput_tensors.size() == 1 && minput_tensors[0].vShape.size() == 4) {
            return static_cast<int>(minput_tensors[0].vShape[1]);
        }
        return -1;
    }

    virtual int inference() = 0;
    virtual int inference(int grpid) = 0;

    int operator()() { return inference(); }
    int operator()(int grpid) { return inference(grpid); }
};
