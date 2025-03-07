//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/ecs.hpp"

#include <sl/meta/storage.hpp>

#include <function2/function2.hpp>

namespace sl::game {

template <typename T>
using storage = meta::persistent_storage<meta::unique_string, T>;

template <typename T>
using array_storage = meta::persistent_array_storage<meta::unique_string, T>;

template <typename Signature, typename Capacity = fu2::capacity_default>
using function = fu2::function_base<
    /*IsOwning=*/true,
    /*IsCopyable=*/false,
    /*Capacity=*/Capacity,
    /*IsThrowing=*/false,
    /*HasStrongExceptGuarantee=*/true,
    /*Signatures=*/Signature>;

template <typename Layer>
concept GameLayerRequirements = requires(Layer& layer) {
    { layer.registry } -> std::same_as<entt::registry&>;
    { layer.root } -> std::convertible_to<entt::entity>;
};

} // namespace sl::game
