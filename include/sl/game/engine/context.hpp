//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/context.hpp"
#include "sl/game/input/system.hpp"
#include "sl/game/time.hpp"

#include <sl/exec/algo/sched/manual.hpp>
#include <sl/exec/coro/async.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/rt/context.hpp>

namespace sl::game::engine {

struct context {
    [[nodiscard]] static context initialize(window_context&& w_ctx, int argc = 0, char** argv = nullptr);

    [[nodiscard]] bool is_ok() const;

    [[nodiscard]] const time_point& time_calculate();

    exec::async<meta::maybe<const time_point&>> next_frame();

public:
    rt::context rt_ctx;
    std::filesystem::path root_path;

    window_context w_ctx;
    std::unique_ptr<input_system> in_sys;

    std::unique_ptr<exec::manual_executor> script_exec;

    time time;
    meta::maybe<time_point> maybe_time_point;
};

} // namespace sl::game::engine
