//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/input/event.hpp"
#include "sl/game/layer.hpp"

#include <sl/gfx/primitives/basis.hpp>

namespace sl::game {

template <typename Layer>
struct input_component {
    using handler_t = component_callback<void(Layer&, entt::entity, const input_events&)>;
    handler_t handler;
};

} // namespace sl::game
