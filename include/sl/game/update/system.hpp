//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/layer.hpp"
#include "sl/game/update/component.hpp"

#include <libassert/assert.hpp>

namespace sl::game {
namespace detail {

template <GameLayerRequirements Layer>
void tree_update_top_down(Layer& layer, update<Layer> update, time_point time_point) {
    std::deque<entt::entity> queue{ layer.root };

    while (!queue.empty()) {
        const entt::entity node_entity = queue.front();
        queue.pop_front();

        if (const node* maybe_node_component = layer.registry.template try_get<node>(node_entity);
            maybe_node_component != nullptr) {
            const node& node_component = *maybe_node_component;
            queue.insert(queue.end(), node_component.children.begin(), node_component.children.end());
        }

        update(layer, node_entity, time_point);
    }
}

template <GameLayerRequirements Layer>
void tree_update_bottom_up(Layer& layer, update<Layer> update, time_point time_point) {
    std::vector<entt::entity> tmp{ layer.root };
    std::vector<entt::entity> accum;

    while (!tmp.empty()) {
        const entt::entity node_entity = tmp.back();
        tmp.pop_back();
        accum.push_back(node_entity);

        if (const node* maybe_node_component = layer.registry.template try_get<node>(node_entity);
            maybe_node_component != nullptr) {
            const node& node_component = *maybe_node_component;
            tmp.insert(tmp.end(), node_component.children.begin(), node_component.children.end());
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
void tree_update_system(tree_update_order order, Layer& layer, update<Layer> update, time_point time_point) {
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

template <GameLayerRequirements Layer>
void update_system(Layer& layer, time_point time_point) {
    auto entities = layer.registry.template view<update<Layer>>();
    for (auto&& [entity, update] : entities.each()) {
        update(layer, entity, time_point);
    }
}

} // namespace sl::game
