#pragma once
#include "npu/runner/ax_model_runner.hpp"

class ax_runner_ax650 : public ax_runner_base
{
protected:
    struct ax_joint_runner_ax650_handle_t *m_handle = nullptr;

public:
    int init(const void *model_data, unsigned int model_size, int devid) override;

    void deinit() override;

    int set_affinity(int id) override;

    int sync_input(int idx) override { return 0; }
    int sync_input(std::string name) override { return 0; }
    int sync_output(int idx) override;
    int sync_output(std::string name) override;

    int sync_input(int grpid, int idx) override { return 0; }
    int sync_input(int grpid, std::string name) override { return 0; }
    int sync_output(int grpid, int idx) override;
    int sync_output(int grpid, std::string name) override;

    int inference() override;
    int inference(int grpid) override;
};
