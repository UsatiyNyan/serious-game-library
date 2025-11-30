//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/context.hpp"

#include <sl/ecs.hpp>

#include <sl/gfx/draw/draw.hpp>
#include <sl/gfx/shader/program.hpp>
#include <sl/gfx/vtx/texture.hpp>
#include <sl/gfx/vtx/vertex_array.hpp>

#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/storage/unique_string.hpp>

#include <glm/vec4.hpp>

namespace sl::game {

struct texture {
    struct id {
        meta::unique_string id;
    };

public:
    gfx::texture tex;
};

struct material {
    struct id {
        meta::unique_string id;
    };
    using tex_or_clr = std::variant<meta::persistent<texture>, glm::vec4>;

public:
    tex_or_clr diffuse;
    tex_or_clr specular;
    float shininess;
};

struct vertex {
    struct id {
        meta::unique_string id;
    };

    // closure for vb/eb
    using draw_type = meta::unique_function<void(gfx::draw&)>;

public:
    gfx::vertex_array va;
    draw_type draw;
};

struct shader {
    struct id {
        meta::unique_string id;
    };

    gfx::shader_program sp;

    using draw_type =
        meta::unique_function<void(const gfx::bound_vertex_array&, vertex::draw_type&, std::span<const entt::entity>)>;
    meta::unique_function<draw_type(const ecs::layer&, const camera_frame&, const gfx::bound_shader_program&)> setup;
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
