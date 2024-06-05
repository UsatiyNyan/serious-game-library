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

#include <entt/entt.hpp>
#include <function2/function2.hpp>

namespace sl::game {

template <typename Signature, typename Capacity = fu2::capacity_default>
using component_callback = fu2::function_base<
    /*IsOwning=*/true,
    /*IsCopyable=*/false,
    /*Capacity=*/Capacity,
    /*IsThrowing=*/false,
    /*HasStrongExceptGuarantee=*/true,
    /*Signatures=*/Signature>;

struct texture_component {
    struct id {
        meta::unique_string id;
    };

    gfx::texture tex;
};

struct vertex_component {
    struct id {
        meta::unique_string id;
    };

    gfx::vertex_array va;
};

struct shader_component {
    struct id {
        meta::unique_string id;
    };

    gfx::shader_program sp;

    component_callback<void(const gfx::bound_shader_program&, entt::registry&)> setup;
    component_callback<
        void(const gfx::bound_shader_program&, const gfx::bound_vertex_array&, entt::registry&, std::span<const entt::entity>)>
        draw;
};

} // namespace sl::game
