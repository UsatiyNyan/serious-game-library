//
// Created by usatiynyan.
//

#pragma once

#include <sl/gfx/ctx.hpp>

#include <sl/meta/conn/dirty.hpp>
#include <sl/meta/lifetime/lazy_eval.hpp>

#include <glm/glm.hpp>

#include <array>

namespace sl::game {

// TODO: my own enums for buttons and actions maybe?

struct keyboard_input_state {
    std::array<meta::dirty<bool>, GLFW_KEY_LAST> pressed{};
    meta::dirty<std::int32_t> mods;
};

struct mouse_input_state {
    std::array<meta::dirty<bool>, GLFW_MOUSE_BUTTON_LAST> pressed{};
    meta::dirty<std::int32_t> mods;
};

struct cursor_input_state {
    meta::dirty<glm::dvec2> prev;
    glm::dvec2 curr;

    tl::optional<glm::dvec2> offset() {
        return prev.then([&curr_value = curr](const glm::dvec2& prev_value) { return curr_value - prev_value; });
    }
};

struct input_state {
    keyboard_input_state keyboard;
    mouse_input_state mouse;
    cursor_input_state cursor;
};

} // namespace sl::game
