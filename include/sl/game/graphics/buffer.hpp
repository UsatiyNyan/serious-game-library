//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/detail/log.hpp"
#include "sl/game/graphics/component/basis.hpp"

#include <sl/ecs/layer.hpp>
#include <sl/gfx/vtx/buffer.hpp>
#include <sl/meta/monad/maybe.hpp>

#include <libassert/assert.hpp>

namespace sl::game {

template <typename SSBOElementT>
concept SSBOElement = requires(
    const ecs::layer& layer,
    basis world,
    entt::entity entity,
    typename SSBOElementT::component_type component
) {
    { SSBOElementT::from(layer, world, entity, component) } -> std::same_as<meta::maybe<SSBOElementT>>;
};

template <SSBOElement SSBOElementT, gfx::buffer_usage buffer_usage = gfx::buffer_usage::dynamic_draw>
[[nodiscard]] gfx::buffer<SSBOElementT, gfx::buffer_type::shader_storage, buffer_usage>
    make_and_initialize_ssbo(std::size_t size) {
    gfx::buffer<SSBOElementT, gfx::buffer_type::shader_storage, buffer_usage> ssbo;
    ssbo.bind().initialize_data(size);
    return ssbo;
}

// returns new size, which has to be set accordingly
template <SSBOElement SSBOElementT, gfx::buffer_usage buffer_usage>
[[nodiscard]] std::uint32_t fill_ssbo(
    const ecs::layer& layer,
    const basis& world,
    gfx::buffer<SSBOElementT, gfx::buffer_type::shader_storage, buffer_usage>& ssbo
) {
    auto bound_ssbo = ssbo.bind();
    auto maybe_mapped_ssbo = bound_ssbo.template map<gfx::buffer_access::write_only>();
    auto mapped_ssbo = *ASSERT_VAL(std::move(maybe_mapped_ssbo));
    auto mapped_ssbo_data = mapped_ssbo.data();
    std::uint32_t size_counter = 0;
    auto view = layer.registry.template view<typename SSBOElementT::component_type>();
    for (const auto& [entity, component] : view.each()) {
        if (const bool enough_capacity = size_counter < mapped_ssbo_data.size();
            !ASSUME_VAL(enough_capacity, mapped_ssbo_data.size())) {
            log::warn("exceeded limit of {} = {}", typeid(SSBOElementT).name(), mapped_ssbo_data.size());
            break;
        }

        if (auto maybe_element = SSBOElementT::from(layer, world, entity, component); maybe_element.has_value()) {
            mapped_ssbo_data[size_counter] = std::move(maybe_element).value();
            ++size_counter;
        }
    }
    return size_counter;
};

} // namespace sl::game
