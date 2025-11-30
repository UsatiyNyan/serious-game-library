//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/input/event.hpp"

#include <sl/ecs/layer.hpp>
#include <sl/meta/func/function.hpp>

namespace sl::game {

struct input {
    meta::unique_function<void(ecs::layer&, const input_events&, entt::entity)> handler;
};

} // namespace sl::game
