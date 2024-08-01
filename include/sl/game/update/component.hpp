//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/layer.hpp"

namespace sl::game {

struct node {
    entt::entity parent = entt::null;
    std::deque<entt::entity> children{}; // amortized insert/remove/iter
};

template <typename Layer>
using update = component_callback<void(Layer&, const rt::time_point&, entt::entity)>;

} // namespace sl::game
