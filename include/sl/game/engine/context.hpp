//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/context.hpp"
#include "sl/game/input/system.hpp"

#include <sl/exec/algo/sched/manual.hpp>
#include <sl/rt.hpp>

namespace sl::game::engine {

struct context {
    [[nodiscard]] static context initialize(window_context&& w_ctx, int argc = 0, char** argv = nullptr);

public:
    rt::context rt_ctx;
    std::filesystem::path root_path;

    window_context w_ctx;
    std::unique_ptr<input_system> in_sys;

    std::unique_ptr<exec::manual_executor> script_exec;

    rt::time time;
};

} // namespace sl::game::engine
