//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/input/event.hpp"
#include "sl/game/layer.hpp"

namespace sl::game {

template <typename Layer>
struct input {
    function<void(Layer&, const input_events&, entt::entity)> handler;
};

} // namespace sl::game
