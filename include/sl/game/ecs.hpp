//
// Created by usatiynyan.
//

#pragma once

#include <libassert/assert.hpp>

#define ENTT_ASSERT(cond, msg) ASSERT((cond), msg)
#include <entt/entt.hpp>
#undef ENTT_ASSERT
