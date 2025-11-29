//
// Created by usatiynyan.
//

#include "sl/game/engine/context.hpp"

#include <sl/exec/algo/sched/start_on.hpp>
#include <sl/exec/coro/await.hpp>

namespace sl::game {

engine_context engine_context::initialize(window_context&& w_ctx, int argc, char** argv) {
    rt::context rt_ctx{ argc, argv };
    auto root_path = rt_ctx.path().parent_path();
    auto in_sys = std::make_unique<input_system>(*w_ctx.window);
    return engine_context{
        .rt_ctx = std::move(rt_ctx),
        .root_path = root_path,
        .w_ctx = std::move(w_ctx),
        .in_sys = std::move(in_sys),
        .script_exec = std::make_unique<exec::manual_executor>(),
        .time{},
        .maybe_time_point{},
    };
}

bool engine_context::is_ok() const { return !w_ctx.current_window.should_close(); }

const time_point& engine_context::time_calculate() { return maybe_time_point.emplace(time.calculate()); }

exec::async<meta::maybe<const time_point&>> engine_context::next_frame() {
    using exec::operator co_await;
    co_await exec::start_on(*script_exec);
    co_return maybe_time_point.map([](const time_point& tp) -> const time_point& { return tp; });
}

} // namespace sl::game
