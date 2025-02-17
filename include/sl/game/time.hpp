//
// Created by usatiynyan.
//

#pragma once

#include <sl/rt/time.hpp>

#include <chrono>

namespace sl::game {

using clock = std::chrono::steady_clock;
using time = rt::time<clock>;
using time_point = rt::time_point<clock>;

} // namespace sl::game
