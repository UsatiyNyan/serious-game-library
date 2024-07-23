//
// Created by usatiynyan.
//

#pragma once

#include <entt/entity/entity.hpp>

#include <deque>

namespace sl::game {

struct node_component {
    entt::entity parent = entt::null;
    std::deque<entt::entity> children{}; // amortized insert/remove/iter
};

} // namespace sl::game
