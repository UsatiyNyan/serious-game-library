//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/render.hpp"
#include "sl/game/input/state.hpp"
#include "sl/game/layer.hpp"

namespace sl::game {

template <typename Layer>
struct input_component {
    using handler_type = component_callback<
        void(const render&, gfx::current_window&, Layer&, entt::entity, input_state&, const rt::time_point&)>;
    handler_type handler;
};

} // namespace sl::game
