//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/time.hpp"

#include <sl/ecs/layer.hpp>
#include <sl/meta/func/function.hpp>

namespace sl::game {

struct node {
    entt::entity parent = entt::null;
    std::deque<entt::entity> children{}; // amortized insert/remove/iter

    static void attach_child(
        ecs::layer& layer,
        entt::entity parent_entity,
        node& parent_node,
        entt::entity child_entity,
        node& child_node
    ) {
        DEBUG_ASSERT(layer.registry.try_get<node>(parent_entity) == &parent_node);
        DEBUG_ASSERT(layer.registry.try_get<node>(child_entity) == &child_node);
        attach_child_impl(parent_entity, parent_node, child_entity, child_node);
    }

    static void attach_child(ecs::layer& layer, entt::entity parent_entity, entt::entity child_entity) {
        node& parent_node = layer.registry.get_or_emplace<node>(parent_entity);
        node& child_node = layer.registry.get_or_emplace<node>(child_entity);
        attach_child_impl(parent_entity, parent_node, child_entity, child_node);
    }

    template <std::size_t children_span>
    static void attach_children(
        ecs::layer& layer,
        entt::entity parent_entity,
        std::span<const entt::entity, children_span> child_entities
    ) {
        node& parent_node = layer.registry.get_or_emplace<node>(parent_entity);
        for (const entt::entity child_entity : child_entities) {
            node& child_node = layer.registry.get_or_emplace<node>(child_entity);
            attach_child_impl(parent_entity, parent_node, child_entity, child_node);
        }
    }

private:
    static entt::entity
        attach_child_impl(entt::entity parent_entity, node& parent_node, entt::entity child_entity, node& child_node) {
        entt::entity prev_parent_entity = std::exchange(child_node.parent, parent_entity);
        if (prev_parent_entity != parent_entity) {
            parent_node.children.push_back(child_entity);
        }
        return prev_parent_entity;
    }
};

using update = meta::unique_function<void(ecs::layer&, entt::entity, time_point)>;

} // namespace sl::game
