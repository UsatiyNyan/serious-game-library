//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/transform.hpp"
#include "sl/game/time.hpp"

#include <sl/ecs/layer.hpp>

namespace sl::game {
namespace detail {

void local_transform_system(ecs::layer& layer, entt::entity entity, time_point time_point [[maybe_unused]]);

} // namespace detail

void local_transform_system(ecs::layer& layer, time_point time_point);

} // namespace sl::game
