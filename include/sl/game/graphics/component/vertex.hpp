//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/ecs.hpp"
#include "sl/game/graphics/context.hpp"
#include "sl/game/layer.hpp"

#include <sl/gfx/draw/draw.hpp>
#include <sl/gfx/shader/program.hpp>
#include <sl/gfx/vtx/texture.hpp>
#include <sl/gfx/vtx/vertex_array.hpp>

#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/storage/unique_string.hpp>

namespace sl::game {

struct texture {
    struct id {
        meta::unique_string id;
    };

    gfx::texture tex;
};

struct material {
    struct id {
        meta::unique_string id;
    };

    meta::persistent<texture> diffuse;
    meta::persistent<texture> specular;
    float shininess;
};

struct vertex {
    struct id {
        meta::unique_string id;
    };

    gfx::vertex_array va;

    // closure for vb/eb
    using draw_type = function<void(gfx::draw&)>;
    draw_type draw;
};

template <typename Layer>
struct shader {
    struct id {
        meta::unique_string id;
    };

    gfx::shader_program sp;

    using draw_type = function<void(const gfx::bound_vertex_array&, vertex::draw_type&, std::span<const entt::entity>)>;
    function<draw_type(Layer&, const camera_frame&, const gfx::bound_shader_program&)> setup;
};

struct primitive {
    struct id {
        meta::unique_string id;
    };

    vertex::id vtx;
    material::id mtl;
    // TODO: shader?
};

struct mesh {
    struct id {
        meta::unique_string id;
    };

    std::vector<primitive::id> primitives;
};

} // namespace sl::game
