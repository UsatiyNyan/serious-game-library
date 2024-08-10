//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/system/render.hpp"

#include "sl/gfx/vtx/buffer.hpp"

#include "sl/game/detail/log.hpp"
#include <libassert/assert.hpp>

namespace sl::game::engine::render::buffer {

template <typename Element, typename Layer>
concept ShaderStorageElementRequirements =
    requires(Layer& layer, entt::entity entity, typename Element::component_type component) {
        { Element::from(layer, entity, component) } -> std::same_as<Element>;
    };

template <typename Element, gfx::buffer_usage buffer_usage = gfx::buffer_usage::dynamic_draw>
[[nodiscard]] auto initialize(std::size_t size) {
    gfx::buffer<Element, gfx::buffer_type::shader_storage, buffer_usage> ssbo;
    ssbo.bind().initialize_data(size);
    return ssbo;
}

// returns new size, which has to be set accordingly
template <GfxLayerRequirements Layer, ShaderStorageElementRequirements<Layer> Element, gfx::buffer_usage buffer_usage>
[[nodiscard]] std::uint32_t
    fill(Layer& layer, gfx::buffer<Element, gfx::buffer_type::shader_storage, buffer_usage>& ssbo) {
    auto bound_ssbo = ssbo.bind();
    auto maybe_mapped_ssbo = bound_ssbo.template map<gfx::buffer_access::write_only>();
    auto mapped_ssbo = *ASSERT_VAL(std::move(maybe_mapped_ssbo));
    auto mapped_ssbo_data = mapped_ssbo.data();
    std::uint32_t size_counter = 0;
    auto view = layer.registry.template view<typename Element::component_type>();
    for (const auto& [entity, component] : view.each()) {
        const bool enough_capacity = size_counter < mapped_ssbo_data.size();
        if (!ASSUME_VAL(enough_capacity, mapped_ssbo_data.size())) {
            log::warn("exceeded limit of {} = {}", typeid(Element).name(), mapped_ssbo_data.size());
            break;
        }
        mapped_ssbo_data[size_counter] = Element::from(layer, entity, component);
        ++size_counter;
    }
    return size_counter;
};

} // namespace sl::game::engine::render::buffer
