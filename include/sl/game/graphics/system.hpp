//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component.hpp"
#include "sl/game/layer.hpp"

#include <entt/entt.hpp>
#include <libassert/assert.hpp>
#include <tsl/robin_map.h>

namespace sl::game {

template <typename Layer>
concept GfxLayerRequirements = GameLayerRequirements<Layer> && requires(Layer& layer) {
    { layer.storage.shader } -> std::same_as<storage<shader_component<Layer>>&>;
    { layer.storage.vertex } -> std::same_as<storage<vertex_component>&>;
};

struct graphics_system {
    gfx::basis world;

    // TODO: framebuffer as a "return value".
    // Im thinking that leaving this as a function is much better.
    // Having to keep track of appearing entities/components (essentially caching)
    // might come with other more subtle performance costs.
    template <GfxLayerRequirements Layer>
    void operator()(graphics_frame& gfx_frame, const graphics_state& gfx_state, Layer& layer) {
        const auto gfx_frame_end = gfx_frame.begin();

        using ve_map_t = tsl::robin_map<
            /* vertex */ meta::unique_string,
            /* entity */ std::vector<entt::entity>>;
        using sv_map_t = tsl::robin_map<
            /* shader */ meta::unique_string,
            /* vertices */ ve_map_t>;

        const sv_map_t sv_map = [&layer] {
            sv_map_t sv_map;
            const auto entities =
                layer.registry.template view<typename shader_component<Layer>::id, vertex_component::id>();
            for (const auto& [entity, shader, vertex] : entities.each()) {
                sv_map[shader.id][vertex.id].push_back(entity);
            }
            return sv_map;
        }();

        const auto camera_entities = layer.registry.template view<camera_component, transform_component>();
        for (const auto& [camera_entity, camera_component, camera_transform] : camera_entities.each()) {
            const camera_state camera_state{
                .world = world,
                .position = camera_transform.tf.tr,
                .projection = camera_component.proj.calculate(gfx_state.frame_buffer_size.get()),
                .view = world.view(camera_transform.tf),
            };

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
                auto draw = shader_component->setup(camera_state, bound_sp, layer);
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
