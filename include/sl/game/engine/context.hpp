//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/vertex.hpp"
#include "sl/game/graphics/context.hpp"
#include "sl/game/input/system.hpp"
#include "sl/game/layer.hpp"

#include <sl/exec/st/executor.hpp>
#include <sl/rt.hpp>

namespace sl::game::engine {

struct layer;

struct storage {
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
    std::unique_ptr<input_system> in_sys;

    std::unique_ptr<exec::st::executor> script_exec;

    rt::time time;
};

} // namespace sl::game::engine
