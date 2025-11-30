//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/basis.hpp"
#include "sl/game/graphics/component/vertex.hpp"
#include "sl/game/graphics/context.hpp"

#include <sl/ecs/layer.hpp>
#include <sl/ecs/resource.hpp>

#include <libassert/assert.hpp>
#include <tsl/robin_map.h>

namespace sl::game {

struct graphics_system {
    enum class system_error : std::uint8_t {
        NO_SHADER_STORAGE,
        NO_VERTEX_STORAGE,
    };

    using vertex_to_entities = tsl::robin_map</* vertex */ meta::unique_string, std::vector<entt::entity>>;
    using shader_to_vertices_to_entities = tsl::robin_map</* shader */ meta::unique_string, vertex_to_entities>;

public:
    // TODO: framebuffer as a "return value".
    template <ecs::ResourceDescriptor ShaderDT, ecs::ResourceDescriptor VertexDT>
    meta::result<meta::unit, system_error> execute(const window_frame& window_frame) & {
        const auto* const maybe_shader_storage = layer.registry.try_get<ecs::resource<ShaderDT>>(layer.root);
        if (maybe_shader_storage == nullptr) {
            return meta::err(system_error::NO_SHADER_STORAGE);
        }
        const ecs::resource<ShaderDT>& shader_storage = *maybe_shader_storage;

        const auto* const maybe_vertex_storage = layer.registry.try_get<ecs::resource<VertexDT>>(layer.root);
        if (maybe_vertex_storage == nullptr) {
            return meta::err(system_error::NO_VERTEX_STORAGE);
        }
        const ecs::resource<ShaderDT>& vertex_storage = *maybe_vertex_storage;

        // Im thinking that recalculating these is much better then .
        // Having to keep track of appearing entities/components (essentially caching) might come with other more subtle
        // performance costs.
        const shader_to_vertices_to_entities sve_map = std::invoke([this] {
            shader_to_vertices_to_entities sve_map;
            const auto entities = layer.registry.template view<typename shader::id, vertex::id>();
            for (const auto& [entity, shader, vertex] : entities.each()) {
                sve_map[shader.id][vertex.id].push_back(entity);
            }
            return sve_map;
        });

        const auto camera_entities = layer.registry.template view<camera, transform>();
        for (const auto& [camera_entity, camera_component, camera_tf] : camera_entities.each()) {
            const auto camera_frame = window_frame.for_camera(world, camera_component, camera_tf);

            for (const auto& [shader_id, ve_map] : sve_map) {
                auto maybe_shader_component = shader_storage.lookup(shader_id);
                if (!ASSUME_VAL(maybe_shader_component.has_value())) {
                    continue;
                }
                meta::persistent<shader> shader_component = std::move(maybe_shader_component).value();

                if (!ASSUME_VAL(shader_component->setup)) {
                    continue;
                }

                const auto bound_sp = shader_component->sp.bind();
                auto draw = shader_component->setup(layer, camera_frame, bound_sp);
                if (!ASSUME_VAL(draw)) {
                    continue;
                }

                for (const auto& [vertex_id, entities] : ve_map) {
                    auto maybe_vertex_component = vertex_storage.lookup(vertex_id);
                    if (!ASSUME_VAL(maybe_vertex_component.has_value())) {
                        continue;
                    }
                    meta::persistent<vertex> vertex_component = std::move(maybe_vertex_component).value();

                    if (!ASSUME_VAL(vertex_component->draw)) {
                        continue;
                    }

                    const auto bound_va = vertex_component->va.bind();
                    draw(bound_va, vertex_component->draw, std::span{ entities });
                }
            }
        }

        return meta::unit{};
    }

public:
    ecs::layer& layer;
    basis world;
};

} // namespace sl::game
