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

#include <tsl/robin_map.h>

namespace sl::game {

void graphics_system::render(layer& layer, rt::time_point time_point) {
    {
        using meta::operator""_hsv;
        const auto [sid, _0] = layer.storage.string.emplace("test"_hsv);
        const auto [sp, _1] = layer.storage.shader.emplace(sid, [&] {
            constexpr auto f = [](entt::registry&, entt::entity) {};
            layer.registry.on_construct<shader_component>().connect<&decltype(f)::operator()>(f);

            return shader_component{
                .sp = *(gfx::shader_program::build(std::span<const gfx::shader>{})),
                .setup = [](const entt::registry&, const gfx::bound_shader_program& bound_sp) {},
            };
        });
    }

    // TODO: pmr linear allocator
    using sv_map_t = tsl::robin_map<
        /* shader */ meta::unique_string,
        /* vertices */ std::vector<meta::unique_string>>;
    using vt_map_t = tsl::robin_map<
        /* vertex */ meta::unique_string,
        /* texture_arrays */ std::vector<meta::unique_string>>;
    using te_map_t = tsl::robin_map<
        /* texture_array */ meta::unique_string,
        /* entity */ std::vector<entt::entity>>;

    sv_map_t sv_map;
    vt_map_t vt_map;
    te_map_t te_map;

    // TODO: instancing
    // TODO: framebuffer - "return value"
    // collect
    const auto entities = layer.registry.view<shader_component::id>();
    for (const auto [entity, shader_id] : entities.each()) {}

    for (const auto& [shader_id, vertex_ids] : sv_map) {
        auto maybe_shader_component = layer.storage.shader.lookup(shader_id);
        if (!ASSUME_VAL(maybe_shader_component.has_value())) {
            continue;
        }
        auto& shader_component = maybe_shader_component.value();
        ASSUME(shader_component->setup && shader_component->submit && shader_component->flush);

        const auto bound_sp = shader_component->sp.bind();
        shader_component->setup(layer.registry, bound_sp);

        for (const auto& vertex_id : vertex_ids) {
            const auto maybe_vertex_component = layer.storage.vertex.lookup(vertex_id);
            const auto vt_it = vt_map.find(vertex_id);
            if (const bool vt_found = vt_it != vt_map.end();
                !ASSUME_VAL(maybe_vertex_component.has_value()) || !ASSUME_VAL(vt_found)) {
                continue;
            }
            const auto& vertex_component = maybe_vertex_component.value();
            const auto& texture_array_ids = vt_it.value();
            const auto bound_va = vertex_component->va.bind();

            for (const auto& texture_array_id : texture_array_ids) { // maybe not texture_array, but texture_ids
                const auto maybe_texture_array_component = layer.storage.texture_array.lookup(texture_array_id);
                const auto te_it = te_map.find(texture_array_id);
                if (const bool te_found = te_it != te_map.end();
                    !ASSUME_VAL(maybe_texture_array_component.has_value()) || !ASSUME_VAL(te_found)) {
                    continue;
                }
                const auto& texture_array_component = maybe_texture_array_component.value();
                const auto& entities = te_it.value();

                shader_component->submit(
                    layer.registry,
                    bound_sp,
                    bound_va,
                    deref_texture_component_array(texture_array_component.span()),
                    std::span{ entities }
                );
            }

            shader_component->flush(layer.registry, bound_sp, bound_va);
        }
    }
}

} // namespace sl::game
