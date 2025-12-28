//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/context.hpp"
#include "sl/game/graphics/system/overlay.hpp"
#include "sl/game/graphics/system/render.hpp"
#include "sl/game/input/system.hpp"
#include "sl/game/time.hpp"

#include <sl/exec/algo/sched/manual.hpp>
#include <sl/exec/algo/sync/serial.hpp>
#include <sl/exec/coro/async.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/rt/context.hpp>

namespace sl::game {

struct engine_context {
    [[nodiscard]] static engine_context initialize(window_context&& w_ctx, int argc = 0, char** argv = nullptr);

    void spin_once(ecs::layer& layer, game::graphics_system& gfx_system, game::overlay_system& overlay_system);

    [[nodiscard]] bool is_ok() const;

    [[nodiscard]] const time_point& time_calculate();

    exec::async<meta::maybe<const time_point&>> next_frame();

public:
    rt::context rt_ctx;
    std::filesystem::path root_path;

    window_context w_ctx;
    std::unique_ptr<input_system> in_sys;

    std::unique_ptr<exec::manual_executor> script_exec;
    std::unique_ptr<exec::serial_executor<>> sync_exec;

    time time;
    meta::maybe<time_point> maybe_time_point;
};

} // namespace sl::game
