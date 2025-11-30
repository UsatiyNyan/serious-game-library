//
// Created by usatiynyan.
//

#include "sl/game/graphics/system/transform.hpp"
#include "sl/game/update/system.hpp"

#include <libassert/assert.hpp>

namespace sl::game {
namespace detail {

void local_transform_system(ecs::layer& layer, entt::entity entity, time_point time_point [[maybe_unused]]) {
    auto* maybe_local_tf = layer.registry.template try_get<local_transform>(entity);
    if (maybe_local_tf == nullptr) {
        DEBUG_ASSERT(
            !layer.registry.template all_of<transform>(entity),
            "all transforms have to have local_transform component in tree structure"
        );
        return;
    }
    meta::dirty<transform>& local_tf = *maybe_local_tf;
    node& node_component = layer.registry.template get<node>(entity);
    local_tf.release()
        .map([&](const transform& changed_local_tf) -> transform {
            const entt::entity parent = node_component.parent;
            if (parent == entt::null) { // root
                return changed_local_tf;
            }
            const transform* maybe_parent_tf = layer.registry.template try_get<transform>(parent);
            const transform& parent_tf = *ASSERT_VAL(maybe_parent_tf, "parent has to have transform", entity, parent);
            return combine(parent_tf, changed_local_tf);
        })
        .map([&](transform new_tf) {
            layer.registry.template emplace_or_replace<transform>(entity, new_tf);

            for (entt::entity child_entity : node_component.children) {
                local_transform* maybe_child_local_tf = layer.registry.template try_get<local_transform>(child_entity);
                if (maybe_child_local_tf == nullptr) {
                    continue;
                }
                local_transform& child_local_tf = *maybe_child_local_tf;

                child_local_tf.set_dirty(true);
            }
        });
}
} // namespace detail

void local_transform_system(ecs::layer& layer, time_point time_point) {
    tree_update_system(tree_update_order::TOP_DOWN, layer, detail::local_transform_system, time_point);
}

} // namespace sl::game
