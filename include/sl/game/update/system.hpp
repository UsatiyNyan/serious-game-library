//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/update/component.hpp"

#include <libassert/assert.hpp>

namespace sl::game {
namespace detail {

template <GameLayerRequirements Layer>
void tree_update_top_down(Layer& layer, update<Layer> update, rt::time_point time_point) {
    std::deque<entt::entity> queue{ layer.root };

    while (!queue.empty()) {
        const entt::entity node_entity = queue.front();
        queue.pop_front();

        update(layer, node_entity, time_point);

        const node* maybe_nc = layer.registry.template try_get<node>(node_entity);
        if (maybe_nc != nullptr) {
            const node& nc = *maybe_nc;
            queue.insert(queue.end(), nc.children.begin(), nc.children.end());
        }
    }
}

template <GameLayerRequirements Layer>
void tree_update_bottom_up(Layer& layer, update<Layer> update, rt::time_point time_point) {
    std::vector<entt::entity> tmp{ layer.root };
    std::vector<entt::entity> accum;

    while (!tmp.empty()) {
        const entt::entity node_entity = tmp.back();
        tmp.pop_back();
        accum.push_back(node_entity);

        const node* maybe_nc = layer.registry.template try_get<node>(node_entity);
        if (maybe_nc != nullptr) {
            const node& nc = *maybe_nc;
            tmp.insert(tmp.end(), nc.children.begin(), nc.children.end());
        }
    }

    for (auto r_it = accum.rbegin(); r_it != accum.rend(); ++r_it) {
        const entt::entity node_entity = *r_it;
        update(layer, node_entity, time_point);
    }
}

} // namespace detail

enum class tree_update_order {
    TOP_DOWN, // visit parent first
    BOTTOM_UP, // visit children first
};

template <GameLayerRequirements Layer>
void tree_update_system(tree_update_order order, Layer& layer, update<Layer> update, rt::time_point time_point) {
    switch (order) {
    case tree_update_order::TOP_DOWN:
        detail::tree_update_top_down(layer, std::move(update), time_point);
        break;
    case tree_update_order::BOTTOM_UP:
        detail::tree_update_bottom_up(layer, std::move(update), time_point);
        break;
    default:
        PANIC();
    }
}

template <typename Layer>
concept UpdateLayerRequirements = GameLayerRequirements<Layer>;

template <UpdateLayerRequirements Layer>
void update_system(Layer& layer, rt::time_point time_point) {
    auto entities = layer.registry.template view<update<Layer>>();
    for (auto&& [entity, update] : entities.each()) {
        update(layer, entity, time_point);
    }
}

} // namespace sl::game
