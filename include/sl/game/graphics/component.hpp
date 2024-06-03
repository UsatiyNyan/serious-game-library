//
// Created by usatiynyan.
//

#pragma once

#include <sl/gfx/shader/program.hpp>
#include <sl/gfx/vtx/texture.hpp>
#include <sl/gfx/vtx/vertex_array.hpp>
#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/storage/unique_string.hpp>
#include <sl/rt/time.hpp>

#include <entt/fwd.hpp>
#include <function2/function2.hpp>
#include <range/v3/view/transform.hpp>

namespace sl::game {
namespace detail {

template <typename Signature>
using component_callback = fu2::function_base<
    /*IsOwning=*/true,
    /*IsCopyable=*/false,
    /*Capacity=*/fu2::capacity_default,
    /*IsThrowing=*/false,
    /*HasStrongExceptGuarantee=*/true,
    /*Signatures=*/Signature>;

} // namespace detail

struct texture_component {
    struct id {
        meta::unique_string id;
    };

    gfx::texture tex;
};

constexpr auto deref_texture_component_array =
    ranges::views::transform([](const meta::persistent<texture_component>& x) -> const gfx::texture& {
        return x->tex;
    });

using deref_texture_component_array_t =
    decltype(deref_texture_component_array(std::declval<std::span<meta::persistent<texture_component>>>()));

struct vertex_component {
    struct id {
        meta::unique_string id;
    };

    gfx::vertex_array va;
    // gfx::buffer<vertex_type, buffer_type::array, buffer_usage::static/dynamic_draw> vb;
    gfx::buffer<std::uint32_t, gfx::buffer_type::element_array, gfx::buffer_usage::static_draw> eb;
};

struct shader_component {
    struct id {
        meta::unique_string id;
    };

    gfx::shader_program sp;
    detail::component_callback<void(entt::registry&, const gfx::bound_shader_program&)> setup;
    detail::component_callback<void(entt::registry&, const gfx::bound_shader_program&, const gfx::bound_vertex_array&, //
                                    deref_texture_component_array_t&&, std::span<const entt::entity>)>
        submit;
    detail::component_callback<void(entt::registry&, const gfx::bound_shader_program&, const gfx::bound_vertex_array&)>
        flush;
};

} // namespace sl::game
