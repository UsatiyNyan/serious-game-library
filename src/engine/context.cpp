//
// Created by usatiynyan.
//

#include "sl/game/engine/context.hpp"
#include <sl/meta/lifetime/lazy_eval.hpp>

#include <libassert/assert.hpp>

namespace sl::game::engine {

meta::unique_string storage::emplace_unique_string(meta::hash_string_view hsv) {
    auto [value, is_emplaced] = string.emplace(hsv);
    ASSERT(is_emplaced);
    return value;
}

meta::persistent<shader<layer>> storage::emplace_unique_shader(meta::unique_string id, game::shader<layer> value) {
    auto [reference, is_emplaced] = shader.emplace(id, meta::identity_eval{ std::move(value) });
    ASSERT(is_emplaced);
    return reference;
}

meta::persistent<vertex> storage::emplace_unique_vertex(meta::unique_string id, game::vertex value) {
    auto [reference, is_emplaced] = vertex.emplace(id, meta::identity_eval{ std::move(value) });
    ASSERT(is_emplaced);
    return reference;
}

meta::persistent<texture> storage::emplace_unique_texture(meta::unique_string id, game::texture value) {
    auto [reference, is_emplaced] = texture.emplace(id, meta::identity_eval{ std::move(value) });
    ASSERT(is_emplaced);
    return reference;
}

meta::persistent<material> storage::emplace_unique_material(meta::unique_string id, game::material value) {
    auto [reference, is_emplaced] = material.emplace(id, meta::identity_eval{ std::move(value) });
    ASSERT(is_emplaced);
    return reference;
}

context context::initialize(window_context&& w_ctx, int argc, char** argv) {
    rt::context rt_ctx{ argc, argv };
    auto root_path = rt_ctx.path().parent_path();
    input_system in_sys{ *w_ctx.window };
    return context{
        .rt_ctx = std::move(rt_ctx),
        .root_path = root_path,
        .w_ctx = std::move(w_ctx),
        .in_sys = std::move(in_sys),
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
