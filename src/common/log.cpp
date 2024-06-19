//
// Created by usatiynyan.
//

#include "sl/game/common/log.hpp"

namespace sl::game {

spdlog::logger& logger() {
    static spdlog::logger logger{ "sl::game" };
    return logger;
}

} // namespace sl::game
