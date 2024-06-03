//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component.hpp"

#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/storage/persistent_array.hpp>
#include <sl/meta/storage/unique_string.hpp>

#include <entt/entt.hpp>

namespace sl::game {

struct layer {
    struct storage {
        meta::unique_string_storage string;
        meta::persistent_storage<meta::unique_string, shader_component> shader;
        meta::persistent_storage<meta::unique_string, vertex_component> vertex;
        meta::persistent_storage<meta::unique_string, texture_component> texture;
        meta::persistent_array_storage<meta::unique_string, meta::persistent<texture_component>> texture_array;
    };
    storage storage;

    entt::registry registry;
};

} // namespace sl::game
