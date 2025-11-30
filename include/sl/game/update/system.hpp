//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/update/component.hpp"

#include <libassert/assert.hpp>
#include <sl/ecs/layer.hpp>
#include <type_traits>

namespace sl::game {

template <typename F>
concept Update = std::is_invocable_r_v<void, F, ecs::layer&, entt::entity, time_point>;

namespace detail {

void tree_update_top_down(ecs::layer& layer, Update auto an_update, time_point time_point) {
    std::deque<entt::entity> queue{ layer.root };

    while (!queue.empty()) {
        const entt::entity node_entity = queue.front();
        queue.pop_front();

        if (const node* maybe_node_component = layer.registry.template try_get<node>(node_entity);
            maybe_node_component != nullptr) {
            const node& node_component = *maybe_node_component;
            queue.insert(queue.end(), node_component.children.begin(), node_component.children.end());
        }

        an_update(layer, node_entity, time_point);
    }
}

void tree_update_bottom_up(ecs::layer& layer, Update auto an_update, time_point time_point) {
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
        an_update(layer, node_entity, time_point);
    }
}

} // namespace detail

enum class tree_update_order {
    TOP_DOWN, // visit parent first
    BOTTOM_UP, // visit children first
};

void tree_update_system(tree_update_order order, ecs::layer& layer, Update auto an_update, time_point time_point) {
    switch (order) {
    case tree_update_order::TOP_DOWN:
        detail::tree_update_top_down(layer, std::move(an_update), time_point);
        break;
    case tree_update_order::BOTTOM_UP:
        detail::tree_update_bottom_up(layer, std::move(an_update), time_point);
        break;
    default:
        UNREACHABLE();
    }
}

inline void update_system(ecs::layer& layer, time_point time_point) {
    auto entities = layer.registry.template view<update>();
    for (auto&& [entity, an_update] : entities.each()) {
        an_update(layer, entity, time_point);
    }
}

} // namespace sl::game
