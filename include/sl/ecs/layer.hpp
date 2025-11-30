//
// Created by usatiynyan.
//

#pragma once

#include "sl/ecs/vendor.hpp"

namespace sl::ecs {

struct layer {
    explicit layer() : registry{}, root{ registry.create() } {}

public:
    entt::registry registry;
    entt::entity root;
};

} // namespace sl::ecs
