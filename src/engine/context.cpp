//
// Created by usatiynyan.
//

#include "sl/game/engine/context.hpp"

#include <sl/meta/lifetime/lazy_eval.hpp>

#include <libassert/assert.hpp>

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
        .script_exec = std::make_unique<exec::st::executor>(),
        .time{},
    };
}

} // namespace sl::game::engine
