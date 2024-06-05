//
// Created by usatiynyan.
//

#pragma once

#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/storage/persistent_array.hpp>
#include <sl/meta/storage/unique_string.hpp>

namespace sl::game {

template <typename T>
using storage = meta::persistent_storage<meta::unique_string, T>;

template <typename T>
using array_storage = meta::persistent_array_storage<meta::unique_string, T>;

} // namespace sl::game
