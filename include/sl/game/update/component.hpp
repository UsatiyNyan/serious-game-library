//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/layer.hpp"

namespace sl::game {

struct node {
    entt::entity parent = entt::null;
    std::deque<entt::entity> children{}; // amortized insert/remove/iter

    template <GameLayerRequirements Layer>
    static void attach_child(
        Layer& layer,
        entt::entity parent_entity,
        node& parent_node,
        entt::entity child_entity,
        node& child_node
    ) {
        entt::registry& registry = layer.registry;
        DEBUG_ASSERT(registry.try_get<node>(parent_entity) == &parent_node);
        DEBUG_ASSERT(registry.try_get<node>(child_entity) == &child_node);
        if (const entt::entity prev_parent_entity = std::exchange(child_node.parent, parent_entity);
            ASSUME_VAL((prev_parent_entity != parent_entity))) {
            parent_node.children.push_back(child_entity);
        }
    }

    template <GameLayerRequirements Layer>
    static void attach_child(Layer& layer, entt::entity parent_entity, entt::entity child_entity) {
        entt::registry& registry = layer.registry;
        node& parent_node = registry.get_or_emplace<node>(parent_entity);
        node& child_node = registry.get_or_emplace<node>(child_entity);
        if (const entt::entity prev_parent_entity = std::exchange(child_node.parent, parent_entity);
            ASSUME_VAL((prev_parent_entity != parent_entity))) {
            parent_node.children.push_back(child_entity);
        }
    }
};

template <GameLayerRequirements Layer>
using update = function<void(Layer&, entt::entity, rt::time_point)>;

} // namespace sl::game
