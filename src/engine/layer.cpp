//
// Created by usatiynyan.
//

#include "sl/game/engine/layer.hpp"

namespace sl::game::engine {

layer layer::create(config cfg) {
    constexpr std::size_t default_string_capacity = 1024;
    constexpr std::size_t default_shader_capacity = 16;
    constexpr std::size_t default_vertex_capacity = 16;
    constexpr std::size_t default_texture_capacity = 16;
    constexpr std::size_t default_material_capacity = 16;
    entt::registry registry;
    entt::entity root = registry.create();
    return layer{
        .storage{
            .string{ cfg.storage.string_capacity.value_or(default_string_capacity) },
            .shader{ cfg.storage.shader_capacity.value_or(default_shader_capacity) },
            .vertex{ cfg.storage.vertex_capacity.value_or(default_vertex_capacity) },
            .texture{ cfg.storage.texture_capacity.value_or(default_texture_capacity) },
            .material{ cfg.storage.material_capacity.value_or(default_material_capacity) },
        },
        .registry = std::move(registry),
        .root = root,
        .world{},
    };
}

} // namespace sl::game::engine
