//
// Created by usatiynyan.
//

#include "sl/game/engine/context.hpp"
#include "sl/game/graphics/system/transform.hpp"
#include "sl/game/update/system.hpp"

#include <sl/exec/algo/sched/start_on.hpp>
#include <sl/exec/coro/await.hpp>

namespace sl::game {

engine_context engine_context::initialize(window_context&& w_ctx, int argc, char** argv) {
    rt::context rt_ctx{ argc, argv };
    auto root_path = rt_ctx.path().parent_path();
    auto in_sys = std::make_unique<input_system>(*w_ctx.window);
    auto script_exec = std::make_unique<exec::manual_executor>();
    auto sync_exec = std::make_unique<exec::serial_executor<>>(*script_exec);
    return engine_context{
        .rt_ctx = std::move(rt_ctx),
        .root_path = root_path,
        .w_ctx = std::move(w_ctx),
        .in_sys = std::move(in_sys),
        .script_exec = std::move(script_exec),
        .sync_exec = std::move(sync_exec),
        .time{},
        .maybe_time_point{},
    };
}

void engine_context::spin_once(
    ecs::layer& layer,
    game::graphics_system& gfx_system,
    game::overlay_system& overlay_system
) {
    // input
    w_ctx.context->poll_events();
    w_ctx.state->frame_buffer_size.release().map([this](const glm::ivec2& frame_buffer_size) {
        w_ctx.current_window.viewport(glm::ivec2{}, frame_buffer_size);
    });
    w_ctx.state->window_content_scale
        .release() //
        .map([&overlay_system](const glm::fvec2& window_content_scale) {
            overlay_system.on_window_content_scale_changed(window_content_scale);
        });
    in_sys->process(layer);

    const game::time_point& time_point = time_calculate();

    // update and execute scripts
    std::ignore = script_exec->execute_batch();
    game::update_system(layer, time_point);

    // transform update
    game::local_transform_system(layer, time_point);

    // render
    const auto window_frame = w_ctx.new_frame();
    gfx_system.execute(window_frame);

    // overlay
    auto imgui_frame = w_ctx.imgui.new_frame();
    overlay_system.execute(imgui_frame);
}

bool engine_context::is_ok() const { return !w_ctx.current_window.should_close(); }

const time_point& engine_context::time_calculate() { return maybe_time_point.emplace(time.calculate()); }

exec::async<meta::maybe<const time_point&>> engine_context::next_frame() {
    using exec::operator co_await;
    co_await exec::start_on(*script_exec);
    co_return maybe_time_point.map([](const time_point& tp) -> const time_point& { return tp; });
}

} // namespace sl::game
