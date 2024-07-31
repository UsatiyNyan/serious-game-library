//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/vertex.hpp"
#include "sl/game/graphics/context.hpp"
#include "sl/game/layer.hpp"

#include <entt/entt.hpp>
#include <libassert/assert.hpp>
#include <tsl/robin_map.h>

namespace sl::game {

template <typename Layer>
concept GfxLayerRequirements = GameLayerRequirements<Layer> && requires(Layer& layer) {
    { layer.storage.shader } -> std::same_as<storage<shader<Layer>>&>;
    { layer.storage.vertex } -> std::same_as<storage<vertex>&>;
};

struct graphics_system {
    // TODO: framebuffer as a "return value".
    // Im thinking that leaving this as a function is much better.
    // Having to keep track of appearing entities/components (essentially caching)
    // might come with other more subtle performance costs.
    template <GfxLayerRequirements Layer>
    void operator()(Layer& layer, const window_frame& window_frame) {
        using ve_map_t = tsl::robin_map<
            /* vertex */ meta::unique_string,
            /* entity */ std::vector<entt::entity>>;
        using sv_map_t = tsl::robin_map<
            /* shader */ meta::unique_string,
            /* vertices */ ve_map_t>;

        const sv_map_t sv_map = [&layer] {
            sv_map_t sv_map;
            const auto entities = layer.registry.template view<typename shader<Layer>::id, vertex::id>();
            for (const auto& [entity, shader, vertex] : entities.each()) {
                sv_map[shader.id][vertex.id].push_back(entity);
            }
            return sv_map;
        }();

        const auto camera_entities = layer.registry.template view<camera, transform>();
        for (const auto& [camera_entity, camera_component, camera_tf] : camera_entities.each()) {
            const auto camera_frame = window_frame.for_camera(camera_component, camera_tf);

            for (const auto& [shader_id, ve_map] : sv_map) {
                auto maybe_shader_component = layer.storage.shader.lookup(shader_id);
                if (!ASSUME_VAL(maybe_shader_component.has_value())) {
                    continue;
                }

                auto& shader_component = maybe_shader_component.value();
                if (!ASSUME_VAL(shader_component->setup)) {
                    continue;
                }

                const auto bound_sp = shader_component->sp.bind();
                auto draw = shader_component->setup(layer, camera_frame, bound_sp);
                if (!ASSUME_VAL(draw)) {
                    continue;
                }

                for (const auto& [vertex_id, entities] : ve_map) {
                    auto maybe_vertex_component = layer.storage.vertex.lookup(vertex_id);
                    if (!ASSUME_VAL(maybe_vertex_component.has_value())) {
                        continue;
                    }

                    auto& vertex_component = maybe_vertex_component.value();
                    if (!ASSUME_VAL(vertex_component->draw)) {
                        continue;
                    }

                    const auto bound_va = vertex_component->va.bind();
                    draw(bound_va, vertex_component->draw, std::span{ entities });
                }
            }
        }
    }
};

} // namespace sl::game
