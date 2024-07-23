//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/node/component.hpp"

#include "sl/game/layer.hpp"

#include <deque>
#include <libassert/assert.hpp>
#include <vector>

namespace sl::game {

namespace detail {

template <GameLayerRequirements Layer, typename F>
void node_system_top_down(Layer& layer, entt::entity root, F&& f) {
    std::deque<entt::entity> queue{ root };

    while (!queue.empty()) {
        const entt::entity node = queue.front();
        queue.pop_front();

        f(layer, node);

        const node_component* maybe_nc = layer.registry.template try_get<node_component>(node);
        if (maybe_nc != nullptr) {
            const node_component& nc = *maybe_nc;
            queue.insert(queue.end(), nc.children.begin(), nc.children.end());
        }
    }
}

template <GameLayerRequirements Layer, typename F>
void node_system_bottom_up(Layer& layer, entt::entity root, F&& f) {
    std::vector<entt::entity> tmp{ root };
    std::vector<entt::entity> accum;

    while (!tmp.empty()) {
        const entt::entity node = tmp.back();
        tmp.pop_back();
        accum.push_back(node);

        const node_component* maybe_nc = layer.registry.template try_get<node_component>(node);
        if (maybe_nc != nullptr) {
            const node_component& nc = *maybe_nc;
            tmp.insert(tmp.end(), nc.children.begin(), nc.children.end());
        }
    }

    for (auto r_it = accum.rbegin(); r_it != accum.rend(); ++r_it) {
        const entt::entity node = *r_it;
        f(layer, node);
    }
}

} // namespace detail

enum class node_system_order {
    TOP_DOWN, // visit parent first
    BOTTOM_UP, // visit children first
};

template <GameLayerRequirements Layer, typename F>
void node_system(node_system_order order, Layer& layer, entt::entity root, F&& f) {
    switch (order) {
    case node_system_order::TOP_DOWN:
        detail::node_system_top_down(layer, root, std::forward<F>(f));
        break;
    case node_system_order::BOTTOM_UP:
        detail::node_system_bottom_up(layer, root, std::forward<F>(f));
        break;
    default:
        PANIC();
    }
}

} // namespace sl::game
