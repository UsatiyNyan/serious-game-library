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

template <GameLayerRequirements Layer>
using update = function<void(Layer&, entt::entity, rt::time_point)>;

} // namespace sl::game
