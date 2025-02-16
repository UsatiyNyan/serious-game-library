//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/transform.hpp"
#include "sl/game/update/system.hpp"

#include <libassert/assert.hpp>

namespace sl::game {
namespace detail {

template <GameLayerRequirements Layer>
void local_transform_system(Layer& layer, entt::entity entity, rt::time_point time_point [[maybe_unused]]) {
    auto* maybe_local_tf = layer.registry.template try_get<local_transform>(entity);
    if (maybe_local_tf == nullptr) {
        DEBUG_ASSERT(
            !layer.registry.template all_of<transform>(entity),
            "all transforms have to have local_transform component in tree structure"
        );
        return;
    }
    meta::dirty<transform>& local_tf = *maybe_local_tf;
    local_tf.release()
        .map([&](const transform& changed_local_tf) -> transform {
            const entt::entity parent = layer.registry.template get<node>(entity).parent;
            if (parent == entt::null) { // root
                return changed_local_tf;
            }
            const transform* maybe_parent_tf = layer.registry.template try_get<transform>(parent);
            const transform& parent_tf = *ASSERT_VAL(maybe_parent_tf, "parent has to have transform", entity, parent);
            return combine(parent_tf, changed_local_tf);
        })
        .map([&](transform new_tf) { layer.registry.template emplace_or_replace<transform>(entity, new_tf); });
}
} // namespace detail

template <GameLayerRequirements Layer>
void local_transform_system(Layer& layer, rt::time_point time_point) {
    tree_update_system(
        tree_update_order::TOP_DOWN, layer, update<Layer>{ detail::local_transform_system<Layer> }, time_point
    );
}

} // namespace sl::game
