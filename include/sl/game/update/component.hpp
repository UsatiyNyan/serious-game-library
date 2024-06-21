//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/layer.hpp"

#include <sl/gfx/primitives/basis.hpp>

namespace sl::game {

template <typename Layer>
struct update_component {
    using handler_type = component_callback<void(Layer&, const gfx::basis&, const rt::time_point&, entt::entity)>;
    handler_type handler;
};

// TODO: hierarchical updates

} // namespace sl::game
