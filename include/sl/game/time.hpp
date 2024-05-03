//
// Created by usatiynyan.
//

#pragma once

#include <sl/meta/lifetime/unique.hpp>

#include <chrono>
#include <utility>

namespace sl::game {

class time : public meta::unique {
public:
    std::chrono::duration<float> calculate_delta() {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<float>{now - std::exchange(now_, now)};
    };

private:
    std::chrono::steady_clock::time_point now_ = std::chrono::steady_clock::now();
};

} // namespace sl::game
