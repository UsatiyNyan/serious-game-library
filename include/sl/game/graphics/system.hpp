//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component.hpp"
#include "sl/game/storage.hpp"

#include <entt/fwd.hpp>

namespace sl::game {

void graphics_system(
    storage<shader_component>& shader_storage,
    storage<vertex_component>& vertex_storage,
    entt::registry& registry
);

} // namespace sl::game
