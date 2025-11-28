//
// Created by usatiynyan.
//

#pragma once

#include "sl/ecs/vendor.hpp"

namespace sl::ecs {

struct layer_view {
    entt::registry& registry;
    entt::entity root;
};

struct layer {
    explicit layer() : registry{}, root{ registry.create() } {}

    [[nodiscard]] layer_view as_view() { return layer_view{ .registry = registry, .root = root }; }

public:
    entt::registry registry;
    entt::entity root;
};

} // namespace sl::ecs
