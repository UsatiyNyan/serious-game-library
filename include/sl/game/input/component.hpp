//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/input/event.hpp"
#include "sl/game/layer.hpp"

namespace sl::game {

template <typename Layer>
struct input_component {
    using handler_type = component_callback<void(Layer&, const input_events&, entt::entity)>;
    handler_type handler;
};

} // namespace sl::game
