//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/engine/loader.hpp"
#include "sl/game/graphics/component/vertex.hpp"
#include "sl/game/layer.hpp"

#include <sl/meta/lifetime/immovable.hpp>

namespace sl::game::engine {

struct layer : meta::immovable {
    struct config {
        struct storage {
            tl::optional<std::size_t> string_capacity;
            tl::optional<std::size_t> shader_capacity;
            tl::optional<std::size_t> vertex_capacity;
            tl::optional<std::size_t> texture_capacity;
            tl::optional<std::size_t> material_capacity;
        } storage;
    };

    layer(exec::executor& executor, config cfg);

public:
    struct storage_type {
        meta::unique_string_storage string;
        game::storage<shader<layer>> shader;
        game::storage<vertex> vertex;
        game::storage<texture> texture;
        game::storage<material> material;
    } storage;

    struct loader_type {
        loader<shader<layer>> shader;
        loader<vertex> vertex;
        loader<texture> texture;
        loader<material> material;
    } loader;

    entt::registry registry{};
    entt::entity root;

    basis world{};
};

} // namespace sl::game::engine
