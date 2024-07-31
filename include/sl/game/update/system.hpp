//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/update/component.hpp"

namespace sl::game {

template <typename Layer>
concept UpdateLayerRequirements = GameLayerRequirements<Layer>;

template <UpdateLayerRequirements Layer>
void update_system(Layer& layer, const basis& world, const rt::time_point& time_point) {
    auto entities = layer.registry.template view<update<Layer>>();
    for (auto&& [entity, update] : entities.each()) {
        update.handler(layer, world, time_point, entity);
    }
}

// TODO: hierarchical updates

} // namespace sl::game
