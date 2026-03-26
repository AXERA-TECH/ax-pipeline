#pragma once
#include "npu/runner/ax_model_runner.hpp"

#include <map>

class ax_runner_axcl : public ax_runner_base
{
protected:
    struct ax_joint_runner_axcl_handle_t *m_handle = nullptr;
    int group_count = 0;
    bool _auto_sync_before_inference = true;
    bool _auto_sync_after_inference = true;

    int sub_init();

public:
    int init(const void *model_data, unsigned int model_size, int devid) override;

    void deinit() override;

    int set_affinity(int id) override;

    int sync_input(int idx) override;
    int sync_input(std::string name) override;
    int sync_output(int idx) override;
    int sync_output(std::string name) override;

    int sync_input(int grpid, int idx) override;
    int sync_input(int grpid, std::string name) override;
    int sync_output(int grpid, int idx) override;
    int sync_output(int grpid, std::string name) override;

    int set_input(int grpid, int idx, unsigned long long int phy_addr, unsigned long size);
    int set_output(int grpid, int idx, unsigned long long int phy_addr, unsigned long size);

    int set_input(int grpid, std::string name, unsigned long long int phy_addr, unsigned long size);
    int set_output(int grpid, std::string name, unsigned long long int phy_addr, unsigned long size);

    int inference() override;
    int inference(int grpid);
};