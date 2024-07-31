//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/basis.hpp"
#include "sl/game/layer.hpp"

namespace sl::game {

template <typename Layer>
struct update {
    using handler_type = component_callback<void(Layer&, const basis&, const rt::time_point&, entt::entity)>;
    handler_type handler;
};

// TODO: hierarchical updates

} // namespace sl::game
