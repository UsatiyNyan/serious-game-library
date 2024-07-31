//
// Created by usatiynyan.
//

#pragma once

#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/storage/persistent_array.hpp>
#include <sl/meta/storage/unique_string.hpp>

#include <entt/entt.hpp>
#include <function2/function2.hpp>
#include <sl/rt/time.hpp>

namespace sl::game {

template <typename T>
using storage = meta::persistent_storage<meta::unique_string, T>;

template <typename T>
using array_storage = meta::persistent_array_storage<meta::unique_string, T>;

template <typename Signature, typename Capacity = fu2::capacity_default>
using component_callback = fu2::function_base<
    /*IsOwning=*/true,
    /*IsCopyable=*/false,
    /*Capacity=*/Capacity,
    /*IsThrowing=*/false,
    /*HasStrongExceptGuarantee=*/true,
    /*Signatures=*/Signature>;

template <typename Layer>
concept GameLayerRequirements = requires(Layer& layer) {
    { layer.registry } -> std::same_as<entt::registry&>;
};

} // namespace sl::game
