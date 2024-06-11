//
// Created by usatiynyan.
//

#include "sl/game/graphics/system.hpp"
#include "sl/game/graphics/component.hpp"
#include "sl/game/input/generic.hpp"

#include <sl/gfx/render/draw.hpp>
#include <sl/gfx/vtx/texture.hpp>
#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/string/hash_view.hpp>

#include <entt/entt.hpp>
#include <tsl/robin_map.h>

namespace sl::game {

// TODO: framebuffer as a "return value".
// Im thinking that leaving this as a function is much better.
// Having to keep track of appearing entities/components (essentially caching)
// might come with other more subtle performance costs.
void graphics_system(
    const bound_render& bound_render,
    storage<shader_component>& shader_storage,
    storage<vertex_component>& vertex_storage,
    entt::registry& registry
) {
    using sv_map_t = tsl::robin_map<
        /* shader */ meta::unique_string,
        /* vertices */ std::vector<meta::unique_string>>;
    using ve_map_t = tsl::robin_map<
        /* vertex */ meta::unique_string,
        /* entity */ std::vector<entt::entity>>;

    sv_map_t sv_map;
    ve_map_t ve_map;

    const auto entities = registry.view<shader_component::id, vertex_component::id>();
    for (const auto& [entity, shader, vertex] : entities.each()) {
        sv_map[shader.id].push_back(vertex.id);
        ve_map[vertex.id].push_back(entity);
    }

    for (const auto& [shader_id, vertex_ids] : sv_map) {
        auto maybe_shader_component = shader_storage.lookup(shader_id);
        if (!ASSUME_VAL(maybe_shader_component.has_value())) {
            continue;
        }

        auto& shader_component = maybe_shader_component.value();
        if (!ASSUME_VAL(shader_component->setup)) {
            continue;
        }

        const auto bound_sp = shader_component->sp.bind();
        auto draw = shader_component->setup(bound_render, bound_sp, registry);
        if (!ASSUME_VAL(draw)) {
            continue;
        }

        for (const auto& vertex_id : vertex_ids) {
            const auto maybe_vertex_component = vertex_storage.lookup(vertex_id);
            const auto ve_it = ve_map.find(vertex_id);
            if (const bool ve_found = ve_it != ve_map.end();
                !ASSUME_VAL(maybe_vertex_component.has_value()) || !ASSUME_VAL(ve_found)) {
                continue;
            }

            const auto& vertex_component = maybe_vertex_component.value();
            const auto& entities = ve_it.value();

            const auto bound_va = vertex_component->va.bind();
            draw(bound_va, std::span{ entities });
        }
    }
}

} // namespace sl::game
