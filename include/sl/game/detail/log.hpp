//
// Created by usatiynyan.
//

#pragma once

#include <spdlog/spdlog.h>

namespace sl::game {

// this is the only place I am willing to have global state
// cuz it's good enough
spdlog::logger& logger();

namespace log {

template <typename... Args>
void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    logger().trace(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    logger().debug(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    logger().info(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    logger().warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    logger().error(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void critical(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    logger().critical(fmt, std::forward<Args>(args)...);
}

} // namespace log

} // namespace sl::game
