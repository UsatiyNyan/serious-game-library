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

layer context::create_root_layer() {
    // TODO: configuration
    constexpr std::size_t default_string_capacity = 1024;
    constexpr std::size_t default_shader_capacity = 16;
    constexpr std::size_t default_vertex_capacity = 16;
    constexpr std::size_t default_texture_capacity = 16;
    constexpr std::size_t default_material_capacity = 16;
    entt::registry registry;
    entt::entity root = registry.create();
    return layer{
        .storage{
            .string{ default_string_capacity },
            .shader{ default_shader_capacity },
            .vertex{ default_vertex_capacity },
            .texture{ default_texture_capacity },
            .material{ default_material_capacity },
        },
        .registry = std::move(registry),
        .root = root,
        .world{},
    };
}

} // namespace sl::game::engine
