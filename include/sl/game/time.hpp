//
// Created by usatiynyan.
//

#pragma once

#include <sl/meta/lifetime/unique.hpp>

#include <chrono>
#include <utility>

namespace sl::game {

struct time_point {
    std::chrono::steady_clock::time_point now;
    std::chrono::steady_clock::duration delta;
    auto now_sec() const { return std::chrono::duration<float>{ now.time_since_epoch() }; }
    auto delta_sec() const { return std::chrono::duration<float>{ delta }; }
};

class time : public meta::unique {
public:
    time_point calculate() {
        const auto now = std::chrono::steady_clock::now();
        const auto delta = now - std::exchange(now_, now);
        return time_point{ now, delta };
    };

private:
    std::chrono::steady_clock::time_point now_ = std::chrono::steady_clock::now();
};

} // namespace sl::game
