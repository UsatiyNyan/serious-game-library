//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/vertex.hpp"
#include "sl/game/layer.hpp"

namespace sl::game::engine {

struct layer {
    struct config {
        struct storage {
            tl::optional<std::size_t> string_capacity;
            tl::optional<std::size_t> shader_capacity;
            tl::optional<std::size_t> vertex_capacity;
            tl::optional<std::size_t> texture_capacity;
            tl::optional<std::size_t> material_capacity;
        } storage;
    };

public:
    static layer create(config cfg);

public:
    struct storage_type {
        meta::unique_string_storage string;
        game::storage<shader<layer>> shader;
        game::storage<vertex> vertex;
        game::storage<texture> texture;
        game::storage<material> material;
    } storage;

    entt::registry registry;
    entt::entity root;

    basis world;
};

} // namespace sl::game::engine
