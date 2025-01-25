//
// Created by usatiynyan.
// TODO:
// - asynchronous rw_mutex

#pragma once

#include "sl/game/layer.hpp"
#include <tl/optional.hpp>
#include <tsl/robin_map.h>

namespace sl::game::engine {

template <typename T>
struct loader {
    explicit loader(game::storage<T>& storage) : storage_{ storage } {}

private:
    game::storage<T>& storage_;
};

} // namespace sl::game::engine
