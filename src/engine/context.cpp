//
// Created by usatiynyan.
//

#include "sl/game/engine/context.hpp"

#include <sl/exec/algo/make/schedule.hpp>
#include <sl/exec/coro/await.hpp>

namespace sl::game::engine {

context context::initialize(window_context&& w_ctx, int argc, char** argv) {
    rt::context rt_ctx{ argc, argv };
    auto root_path = rt_ctx.path().parent_path();
    auto in_sys = std::make_unique<input_system>(*w_ctx.window);
    return context{
        .rt_ctx = std::move(rt_ctx),
        .root_path = root_path,
        .w_ctx = std::move(w_ctx),
        .in_sys = std::move(in_sys),
        .script_exec = std::make_unique<exec::manual_executor>(),
        .time{},
        .maybe_time_point{},
    };
}

bool context::is_ok() const { return !w_ctx.current_window.should_close(); }

const time_point& context::time_calculate() { return maybe_time_point.emplace(time.calculate()); }

exec::async<meta::maybe<const time_point&>> context::next_frame() {
    co_await exec::schedule(*script_exec);
    co_return maybe_time_point.map([](const time_point& tp) -> const time_point& { return tp; });
}

} // namespace sl::game::engine
