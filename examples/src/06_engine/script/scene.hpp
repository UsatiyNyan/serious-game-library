//
// Created by usatiynyan.
//

#pragma once

#include "box_entities.hpp"
#include "global_entity.hpp"
#include "imported_entities.hpp"
#include "light_entities.hpp"
#include "player_entity.hpp"

#include "../common.hpp"

namespace script {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
constexpr std::array cube_vertices{
    // verts                 | normals            | tex coords
    // front face
    VNT{ +0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +0.0f, +1.0f }, // top left
    // right face
    VNT{ +0.5f, +0.5f, +0.5f, +1.0f, +0.0f, +0.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, -0.5f, +0.5f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    VNT{ +0.5f, -0.5f, -0.5f, +1.0f, +0.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    VNT{ +0.5f, +0.5f, -0.5f, +1.0f, +0.0f, +0.0f, +0.0f, +1.0f }, // top left
    // back face
    VNT{ +0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +0.0f, +1.0f }, // top left
    // left face
    VNT{ -0.5f, +0.5f, -0.5f, -1.0f, +0.0f, +0.0f, +1.0f, +1.0f }, // top right
    VNT{ -0.5f, -0.5f, -0.5f, -1.0f, +0.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, -0.5f, +0.5f, -1.0f, +0.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, +0.5f, +0.5f, -1.0f, +0.0f, +0.0f, +0.0f, +1.0f }, // top left
    // top face
    VNT{ +0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f }, // top left
    // bottom face
    VNT{ +0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f, +0.0f, +1.0f }, // top left
};
#pragma GCC diagnostic pop

constexpr std::array cube_indices{
    0u,  1u,  3u,  1u,  2u,  3u, // Front face
    4u,  5u,  7u,  5u,  6u,  7u, // Right face
    8u,  9u,  11u, 9u,  10u, 11u, // Back face
    12u, 13u, 15u, 13u, 14u, 15u, // Left face
    16u, 17u, 19u, 17u, 18u, 19u, // Top face
    20u, 21u, 23u, 21u, 22u, 23u, // Bottom face
};

inline exec::async<void> create_scene(engine::engine_context& e_ctx, engine::layer& layer) {
    auto example_ctx = example_context::create(e_ctx);

    // common vvv
    const auto cube_vertex_id = "cube.vertex"_us(layer.storage.string);
    std::ignore = co_await layer.loader.vertex.emplace(
        cube_vertex_id, create_vertex(std::span(cube_vertices), std::span(cube_indices))
    );
    // common ^^^

    const entt::entity global_entity = co_await spawn_global_entity(e_ctx, layer);
    const entt::entity player_entity = co_await spawn_player_entity(e_ctx, layer);
    game::node::attach_child(layer, global_entity, player_entity);

    const std::vector<entt::entity> box_entities = co_await spawn_box_entities(e_ctx, layer, example_ctx);
    game::node::attach_children(layer, global_entity, std::span(box_entities));

    const std::vector<entt::entity> light_entities = co_await spawn_light_entities(e_ctx, layer, example_ctx);
    game::node::attach_children(layer, global_entity, std::span(light_entities));

    const entt::entity imported_entity = co_await spawn_imported_entity(e_ctx, layer, example_ctx, "meshes/cube.gltf");
    game::node::attach_child(layer, global_entity, imported_entity);
}

} // namespace script
