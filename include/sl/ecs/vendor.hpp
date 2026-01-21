//
// Created by usatiynyan.
//

#pragma once

#include <sl/meta/assert.hpp>

#define ENTT_ASSERT(cond, msg) ASSERT((cond), msg)
#include <entt/entt.hpp>
#undef ENTT_ASSERT

namespace entt {

template <typename C, typename Traits>
auto& operator<<(std::basic_ostream<C, Traits>& os, const entity& e) {
    return os << to_integral(e);
}

} // namespace entt
