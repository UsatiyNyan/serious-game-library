//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/vertex.hpp"
#include "sl/game/graphics/context.hpp"
#include "sl/game/input/system.hpp"
#include "sl/game/layer.hpp"

#include <sl/rt.hpp>

namespace sl::game::engine {

struct layer;

struct storage {
    // will ASSERT if not unique
    [[nodiscard]] meta::unique_string emplace_unique_string(meta::hash_string_view hsv);
    // will ASSERT if not unique
    [[nodiscard]] meta::persistent<shader<layer>>
        emplace_unique_shader(meta::unique_string id, game::shader<layer> value);
    // will ASSERT if not unique
    [[nodiscard]] meta::persistent<vertex> emplace_unique_vertex(meta::unique_string id, game::vertex value);
    // will ASSERT if not unique
    [[nodiscard]] meta::persistent<texture> emplace_unique_texture(meta::unique_string id, game::texture value);
    // will ASSERT if not unique
    [[nodiscard]] meta::persistent<material> emplace_unique_material(meta::unique_string id, game::material value);

public:
    meta::unique_string_storage string;
    game::storage<shader<layer>> shader;
    game::storage<vertex> vertex;
    game::storage<texture> texture;
    game::storage<material> material;
};

struct layer {
    storage storage;

    entt::registry registry;
    entt::entity root;

    basis world;
};

struct context {
    [[nodiscard]] static context initialize(window_context&& w_ctx, int argc = 0, char** argv = nullptr);
    [[nodiscard]] layer create_root_layer(); // TODO: storage configuration

public:
    rt::context rt_ctx;
    std::filesystem::path root_path;

    window_context w_ctx;
    input_system in_sys;

    rt::time time{};
};

} // namespace sl::game::engine
