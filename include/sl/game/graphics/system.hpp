//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component.hpp"
#include "sl/game/graphics/render.hpp"
#include "sl/game/storage.hpp"

#include <entt/entt.hpp>
#include <libassert/assert.hpp>
#include <tsl/robin_map.h>

namespace sl::game {

template <typename Layer>
concept GfxLayerRequirements = requires(Layer& layer) {
    { layer.registry } -> std::same_as<entt::registry&>;
    { layer.storage.shader } -> std::same_as<storage<shader_component<Layer>>&>;
    { layer.storage.vertex } -> std::same_as<storage<vertex_component>&>;
};

// TODO: framebuffer as a "return value".
// Im thinking that leaving this as a function is much better.
// Having to keep track of appearing entities/components (essentially caching)
// might come with other more subtle performance costs.
template <GfxLayerRequirements Layer>
void graphics_system(const bound_render& bound_render, Layer& layer) {
    using sv_map_t = tsl::robin_map<
        /* shader */ meta::unique_string,
        /* vertices */ std::vector<meta::unique_string>>;
    using ve_map_t = tsl::robin_map<
        /* vertex */ meta::unique_string,
        /* entity */ std::vector<entt::entity>>;

    sv_map_t sv_map;
    ve_map_t ve_map;

    const auto entities = layer.registry.template view<typename shader_component<Layer>::id, vertex_component::id>();
    for (const auto& [entity, shader, vertex] : entities.each()) {
        sv_map[shader.id].push_back(vertex.id);
        ve_map[vertex.id].push_back(entity);
    }

    for (const auto& [shader_id, vertex_ids] : sv_map) {
        auto maybe_shader_component = layer.storage.shader.lookup(shader_id);
        if (!ASSUME_VAL(maybe_shader_component.has_value())) {
            continue;
        }

        auto& shader_component = maybe_shader_component.value();
        if (!ASSUME_VAL(shader_component->setup)) {
            continue;
        }

        const auto bound_sp = shader_component->sp.bind();
        auto draw = shader_component->setup(bound_render, bound_sp, layer);
        if (!ASSUME_VAL(draw)) {
            continue;
        }

        for (const auto& vertex_id : vertex_ids) {
            auto maybe_vertex_component = layer.storage.vertex.lookup(vertex_id);
            const auto ve_it = ve_map.find(vertex_id);
            if (const bool ve_found = ve_it != ve_map.end();
                !ASSUME_VAL(maybe_vertex_component.has_value()) || !ASSUME_VAL(ve_found)) {
                continue;
            }

            auto& vertex_component = maybe_vertex_component.value();
            const auto& vertex_entities = ve_it.value();

            const auto bound_va = vertex_component->va.bind();
            draw(bound_va, vertex_component->draw, std::span{ vertex_entities });
        }
    }
}

} // namespace sl::game
