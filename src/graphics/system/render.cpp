//
// Created by usatiynyan.
//

#include "sl/game/graphics/system/render.hpp"
#include "sl/game/detail/log.hpp"
#include "sl/game/graphics/component/vertex.hpp"

#include <sl/ecs/resource.hpp>

#include <sl/meta/assert.hpp>
#include <tsl/robin_map.h>

namespace sl::game {

meta::result<meta::unit, graphics_system::error_type> graphics_system::execute(const window_frame& a_window_frame) & {
    using vertex_to_entities = tsl::robin_map</* vertex */ meta::unique_string, std::vector<entt::entity>>;
    using shader_to_vertices_to_entities = tsl::robin_map</* shader */ meta::unique_string, vertex_to_entities>;

    auto* const maybe_shader_resource = layer.registry.try_get<ecs::resource<shader>::ptr_type>(layer.root);
    if (maybe_shader_resource == nullptr) {
        log::trace("no shader storage");
        return meta::err(error_type::NO_SHADER_STORAGE);
    }
    auto& shader_resource = **maybe_shader_resource;

    auto* const maybe_vertex_resource = layer.registry.try_get<ecs::resource<vertex>::ptr_type>(layer.root);
    if (maybe_vertex_resource == nullptr) {
        log::trace("no vertex storage");
        return meta::err(error_type::NO_VERTEX_STORAGE);
    }
    auto& vertex_resource = **maybe_vertex_resource;

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
    if (sve_map.empty()) {
        log::trace("no entities with shader::id and vertex::id found");
    }

    const auto camera_entities = layer.registry.template view<camera, transform>();
    if (camera_entities.size_hint() == 0) {
        log::trace("no camera entities found");
    }

    for (const auto& [camera_entity, camera_component, camera_tf] : camera_entities.each()) {
        const auto camera_frame = a_window_frame.for_camera(world, camera_component, camera_tf);

        for (const auto& [shader_id, ve_map] : sve_map) {
            auto maybe_shader_component = shader_resource.lookup_unsafe(shader_id);
            if (!maybe_shader_component.has_value()) {
                log::trace("shader.id={} not found", shader_id.string_view());
                continue;
            }
            meta::persistent<shader> shader_component = std::move(maybe_shader_component).value();
            ASSERT(shader_component->setup);

            const auto bound_sp = shader_component->sp.bind();
            auto draw = shader_component->setup(layer, camera_frame, bound_sp);
            ASSERT(draw);

            for (const auto& [vertex_id, entities] : ve_map) {
                auto maybe_vertex_component = vertex_resource.lookup_unsafe(vertex_id);
                if (!maybe_vertex_component.has_value()) {
                    log::trace("vertex.id={} not found", vertex_id.string_view());
                    continue;
                }
                meta::persistent<vertex> vertex_component = std::move(maybe_vertex_component).value();
                ASSERT(vertex_component->draw);

                const auto bound_va = vertex_component->va.bind();
                draw(bound_va, vertex_component->draw, std::span{ entities });
            }
        }
    }

    return meta::unit{};
}

} // namespace sl::game
