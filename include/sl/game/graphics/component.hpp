//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/context.hpp"
#include "sl/game/layer.hpp"

#include <sl/gfx/primitives.hpp>
#include <sl/gfx/render/draw.hpp>
#include <sl/gfx/render/projection.hpp>
#include <sl/gfx/shader/program.hpp>
#include <sl/gfx/vtx/texture.hpp>
#include <sl/gfx/vtx/vertex_array.hpp>

#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/storage/unique_string.hpp>

#include <sl/rt/time.hpp>

#include <entt/entt.hpp>

namespace sl::game {

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

    // closure for vb/eb
    using draw_type = component_callback<void(gfx::draw&)>;
    draw_type draw;
};

template <typename T, gfx::buffer_type type, gfx::buffer_usage usage>
struct buffer_component {
    struct id {
        meta::unique_string id;
    };
    gfx::buffer<T, type, usage> b;
};

template <typename Layer>
struct shader_component {
    struct id {
        meta::unique_string id;
    };

    gfx::shader_program sp;

    using draw_type = component_callback<
        void(const gfx::bound_vertex_array&, vertex_component::draw_type&, std::span<const entt::entity>)>;
    component_callback<draw_type(Layer&, const camera_state&, const gfx::bound_shader_program&)> setup;
};

struct transform_component {
    gfx::transform tf;

    [[nodiscard]] glm::vec3 direction(const gfx::basis& world) const { return tf.rot * world.forward(); }
};

struct camera_component {
    gfx::projection proj;
};

} // namespace sl::game
