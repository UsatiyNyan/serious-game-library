//
// Created by usatiynyan.
//

#include "sl/game/detail/log.hpp"

namespace sl::game {

spdlog::logger& logger() {
    static spdlog::logger logger{ "sl::game" };
    return logger;
}

} // namespace sl::game
