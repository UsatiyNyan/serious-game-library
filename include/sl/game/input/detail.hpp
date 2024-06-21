//
// Created by usatiynyan.
//
// TODO: maybe move to gfx library and change signals

#pragma once

#include <sl/gfx/ctx.hpp>
#include <sl/meta/enum/flag.hpp>

namespace sl::game::detail {

enum class input_event_action : std::uint8_t {
    RELEASE,
    PRESS,
    REPEAT,
    ENUM_END,
};

constexpr input_event_action input_event_action_from_glfw(int action) {
    switch (action) {
    case GLFW_RELEASE:
        return input_event_action::RELEASE;
    case GLFW_PRESS:
        return input_event_action::PRESS;
    case GLFW_REPEAT:
        return input_event_action::REPEAT;
    default:
        return input_event_action::ENUM_END;
    }
}

enum class input_event_mod : std::uint8_t {
    NONE = 0,
    SHIFT = GLFW_MOD_SHIFT,
    CONTROL = GLFW_MOD_CONTROL,
    ALT = GLFW_MOD_ALT,
    SUPER = GLFW_MOD_SUPER,
    CAPS_LOCK = GLFW_MOD_CAPS_LOCK,
    NUM_LOCK = GLFW_MOD_NUM_LOCK,
};

using input_event_mods = meta::enum_flag<input_event_mod>;

constexpr input_event_mods input_event_mods_from_glfw(int mods) {
    return input_event_mods{ static_cast<std::uint8_t>(mods) };
}

enum class keyboard_input_event : std::uint8_t {
    /* Printable keys */
    SPACE,
    APOSTROPHE,
    COMMA,
    MINUS,
    PERIOD,
    SLASH,
    NR_0, // NR as in NumbeR
    NR_1,
    NR_2,
    NR_3,
    NR_4,
    NR_5,
    NR_6,
    NR_7,
    NR_8,
    NR_9,
    SEMICOLON,
    EQUAL,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LEFT_BRACKET,
    BACKSLASH,
    RIGHT_BRACKET,
    GRAVE_ACCENT,
    WORLD_1,
    WORLD_2,

    /* Function keys */
    ESCAPE,
    ENTER,
    TAB,
    BACKSPACE,
    INSERT,
    DELETE,
    RIGHT,
    LEFT,
    DOWN,
    UP,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    CAPS_LOCK,
    SCROLL_LOCK,
    NUM_LOCK,
    PRINT_SCREEN,
    PAUSE,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    F25,
    KP_0,
    KP_1,
    KP_2,
    KP_3,
    KP_4,
    KP_5,
    KP_6,
    KP_7,
    KP_8,
    KP_9,
    KP_DECIMAL,
    KP_DIVIDE,
    KP_MULTIPLY,
    KP_SUBTRACT,
    KP_ADD,
    KP_ENTER,
    KP_EQUAL,
    LEFT_SHIFT,
    LEFT_CONTROL,
    LEFT_ALT,
    LEFT_SUPER,
    RIGHT_SHIFT,
    RIGHT_CONTROL,
    RIGHT_ALT,
    RIGHT_SUPER,
    MENU,
    ENUM_END,
};

constexpr keyboard_input_event keyboard_input_event_from_glfw(int key) {
    static_assert(GLFW_KEY_LAST == GLFW_KEY_MENU);
    switch (key) {
    case GLFW_KEY_SPACE:
        return keyboard_input_event::SPACE;
    case GLFW_KEY_APOSTROPHE:
        return keyboard_input_event::APOSTROPHE;
    case GLFW_KEY_COMMA:
        return keyboard_input_event::COMMA;
    case GLFW_KEY_MINUS:
        return keyboard_input_event::MINUS;
    case GLFW_KEY_PERIOD:
        return keyboard_input_event::PERIOD;
    case GLFW_KEY_SLASH:
        return keyboard_input_event::SLASH;
    case GLFW_KEY_0:
        return keyboard_input_event::NR_0;
    case GLFW_KEY_1:
        return keyboard_input_event::NR_1;
    case GLFW_KEY_2:
        return keyboard_input_event::NR_2;
    case GLFW_KEY_3:
        return keyboard_input_event::NR_3;
    case GLFW_KEY_4:
        return keyboard_input_event::NR_4;
    case GLFW_KEY_5:
        return keyboard_input_event::NR_5;
    case GLFW_KEY_6:
        return keyboard_input_event::NR_6;
    case GLFW_KEY_7:
        return keyboard_input_event::NR_7;
    case GLFW_KEY_8:
        return keyboard_input_event::NR_8;
    case GLFW_KEY_9:
        return keyboard_input_event::NR_9;
    case GLFW_KEY_SEMICOLON:
        return keyboard_input_event::SEMICOLON;
    case GLFW_KEY_EQUAL:
        return keyboard_input_event::EQUAL;
    case GLFW_KEY_A:
        return keyboard_input_event::A;
    case GLFW_KEY_B:
        return keyboard_input_event::B;
    case GLFW_KEY_C:
        return keyboard_input_event::C;
    case GLFW_KEY_D:
        return keyboard_input_event::D;
    case GLFW_KEY_E:
        return keyboard_input_event::E;
    case GLFW_KEY_F:
        return keyboard_input_event::F;
    case GLFW_KEY_G:
        return keyboard_input_event::G;
    case GLFW_KEY_H:
        return keyboard_input_event::H;
    case GLFW_KEY_I:
        return keyboard_input_event::I;
    case GLFW_KEY_J:
        return keyboard_input_event::J;
    case GLFW_KEY_K:
        return keyboard_input_event::K;
    case GLFW_KEY_L:
        return keyboard_input_event::L;
    case GLFW_KEY_M:
        return keyboard_input_event::M;
    case GLFW_KEY_N:
        return keyboard_input_event::N;
    case GLFW_KEY_O:
        return keyboard_input_event::O;
    case GLFW_KEY_P:
        return keyboard_input_event::P;
    case GLFW_KEY_Q:
        return keyboard_input_event::Q;
    case GLFW_KEY_R:
        return keyboard_input_event::R;
    case GLFW_KEY_S:
        return keyboard_input_event::S;
    case GLFW_KEY_T:
        return keyboard_input_event::T;
    case GLFW_KEY_U:
        return keyboard_input_event::U;
    case GLFW_KEY_V:
        return keyboard_input_event::V;
    case GLFW_KEY_W:
        return keyboard_input_event::W;
    case GLFW_KEY_X:
        return keyboard_input_event::X;
    case GLFW_KEY_Y:
        return keyboard_input_event::Y;
    case GLFW_KEY_Z:
        return keyboard_input_event::Z;
    case GLFW_KEY_LEFT_BRACKET:
        return keyboard_input_event::LEFT_BRACKET;
    case GLFW_KEY_BACKSLASH:
        return keyboard_input_event::BACKSLASH;
    case GLFW_KEY_RIGHT_BRACKET:
        return keyboard_input_event::RIGHT_BRACKET;
    case GLFW_KEY_GRAVE_ACCENT:
        return keyboard_input_event::GRAVE_ACCENT;
    case GLFW_KEY_WORLD_1:
        return keyboard_input_event::WORLD_1;
    case GLFW_KEY_WORLD_2:
        return keyboard_input_event::WORLD_2;

    case GLFW_KEY_ESCAPE:
        return keyboard_input_event::ESCAPE;
    case GLFW_KEY_ENTER:
        return keyboard_input_event::ENTER;
    case GLFW_KEY_TAB:
        return keyboard_input_event::TAB;
    case GLFW_KEY_BACKSPACE:
        return keyboard_input_event::BACKSPACE;
    case GLFW_KEY_INSERT:
        return keyboard_input_event::INSERT;
    case GLFW_KEY_DELETE:
        return keyboard_input_event::DELETE;
    case GLFW_KEY_RIGHT:
        return keyboard_input_event::RIGHT;
    case GLFW_KEY_LEFT:
        return keyboard_input_event::LEFT;
    case GLFW_KEY_DOWN:
        return keyboard_input_event::DOWN;
    case GLFW_KEY_UP:
        return keyboard_input_event::UP;
    case GLFW_KEY_PAGE_UP:
        return keyboard_input_event::PAGE_UP;
    case GLFW_KEY_PAGE_DOWN:
        return keyboard_input_event::PAGE_DOWN;
    case GLFW_KEY_HOME:
        return keyboard_input_event::HOME;
    case GLFW_KEY_END:
        return keyboard_input_event::END;
    case GLFW_KEY_CAPS_LOCK:
        return keyboard_input_event::CAPS_LOCK;
    case GLFW_KEY_SCROLL_LOCK:
        return keyboard_input_event::SCROLL_LOCK;
    case GLFW_KEY_NUM_LOCK:
        return keyboard_input_event::NUM_LOCK;
    case GLFW_KEY_PRINT_SCREEN:
        return keyboard_input_event::PRINT_SCREEN;
    case GLFW_KEY_PAUSE:
        return keyboard_input_event::PAUSE;
    case GLFW_KEY_F1:
        return keyboard_input_event::F1;
    case GLFW_KEY_F2:
        return keyboard_input_event::F2;
    case GLFW_KEY_F3:
        return keyboard_input_event::F3;
    case GLFW_KEY_F4:
        return keyboard_input_event::F4;
    case GLFW_KEY_F5:
        return keyboard_input_event::F5;
    case GLFW_KEY_F6:
        return keyboard_input_event::F6;
    case GLFW_KEY_F7:
        return keyboard_input_event::F7;
    case GLFW_KEY_F8:
        return keyboard_input_event::F8;
    case GLFW_KEY_F9:
        return keyboard_input_event::F9;
    case GLFW_KEY_F10:
        return keyboard_input_event::F10;
    case GLFW_KEY_F11:
        return keyboard_input_event::F11;
    case GLFW_KEY_F12:
        return keyboard_input_event::F12;
    case GLFW_KEY_F13:
        return keyboard_input_event::F13;
    case GLFW_KEY_F14:
        return keyboard_input_event::F14;
    case GLFW_KEY_F15:
        return keyboard_input_event::F15;
    case GLFW_KEY_F16:
        return keyboard_input_event::F16;
    case GLFW_KEY_F17:
        return keyboard_input_event::F17;
    case GLFW_KEY_F18:
        return keyboard_input_event::F18;
    case GLFW_KEY_F19:
        return keyboard_input_event::F19;
    case GLFW_KEY_F20:
        return keyboard_input_event::F20;
    case GLFW_KEY_F21:
        return keyboard_input_event::F21;
    case GLFW_KEY_F22:
        return keyboard_input_event::F22;
    case GLFW_KEY_F23:
        return keyboard_input_event::F23;
    case GLFW_KEY_F24:
        return keyboard_input_event::F24;
    case GLFW_KEY_F25:
        return keyboard_input_event::F25;
    case GLFW_KEY_KP_0:
        return keyboard_input_event::KP_0;
    case GLFW_KEY_KP_1:
        return keyboard_input_event::KP_1;
    case GLFW_KEY_KP_2:
        return keyboard_input_event::KP_2;
    case GLFW_KEY_KP_3:
        return keyboard_input_event::KP_3;
    case GLFW_KEY_KP_4:
        return keyboard_input_event::KP_4;
    case GLFW_KEY_KP_5:
        return keyboard_input_event::KP_5;
    case GLFW_KEY_KP_6:
        return keyboard_input_event::KP_6;
    case GLFW_KEY_KP_7:
        return keyboard_input_event::KP_7;
    case GLFW_KEY_KP_8:
        return keyboard_input_event::KP_8;
    case GLFW_KEY_KP_9:
        return keyboard_input_event::KP_9;
    case GLFW_KEY_KP_DECIMAL:
        return keyboard_input_event::KP_DECIMAL;
    case GLFW_KEY_KP_DIVIDE:
        return keyboard_input_event::KP_DIVIDE;
    case GLFW_KEY_KP_MULTIPLY:
        return keyboard_input_event::KP_MULTIPLY;
    case GLFW_KEY_KP_SUBTRACT:
        return keyboard_input_event::KP_SUBTRACT;
    case GLFW_KEY_KP_ADD:
        return keyboard_input_event::KP_ADD;
    case GLFW_KEY_KP_ENTER:
        return keyboard_input_event::KP_ENTER;
    case GLFW_KEY_KP_EQUAL:
        return keyboard_input_event::KP_EQUAL;
    case GLFW_KEY_LEFT_SHIFT:
        return keyboard_input_event::LEFT_SHIFT;
    case GLFW_KEY_LEFT_CONTROL:
        return keyboard_input_event::LEFT_CONTROL;
    case GLFW_KEY_LEFT_ALT:
        return keyboard_input_event::LEFT_ALT;
    case GLFW_KEY_LEFT_SUPER:
        return keyboard_input_event::LEFT_SUPER;
    case GLFW_KEY_RIGHT_SHIFT:
        return keyboard_input_event::RIGHT_SHIFT;
    case GLFW_KEY_RIGHT_CONTROL:
        return keyboard_input_event::RIGHT_CONTROL;
    case GLFW_KEY_RIGHT_ALT:
        return keyboard_input_event::RIGHT_ALT;
    case GLFW_KEY_RIGHT_SUPER:
        return keyboard_input_event::RIGHT_SUPER;
    case GLFW_KEY_MENU:
        return keyboard_input_event::MENU;

    case GLFW_KEY_UNKNOWN:
    default:
        return keyboard_input_event::ENUM_END;
    }
}

enum class mouse_button_input_event : std::uint8_t {
    MB_1,
    MB_2,
    MB_3,
    MB_4,
    MB_5,
    MB_6,
    MB_7,
    MB_8,
    ENUM_END,
    LEFT = MB_1,
    RIGHT = MB_2,
    MIDDLE = MB_3,
};

constexpr mouse_button_input_event mouse_button_input_event_from_glfw(int button) {
    static_assert(GLFW_MOUSE_BUTTON_LAST == GLFW_MOUSE_BUTTON_8);
    switch (button) {
    case GLFW_MOUSE_BUTTON_1:
        return mouse_button_input_event::MB_1;
    case GLFW_MOUSE_BUTTON_2:
        return mouse_button_input_event::MB_2;
    case GLFW_MOUSE_BUTTON_3:
        return mouse_button_input_event::MB_3;
    case GLFW_MOUSE_BUTTON_4:
        return mouse_button_input_event::MB_4;
    case GLFW_MOUSE_BUTTON_5:
        return mouse_button_input_event::MB_5;
    case GLFW_MOUSE_BUTTON_6:
        return mouse_button_input_event::MB_6;
    case GLFW_MOUSE_BUTTON_7:
        return mouse_button_input_event::MB_7;
    case GLFW_MOUSE_BUTTON_8:
        return mouse_button_input_event::MB_8;
    default:
        return mouse_button_input_event::ENUM_END;
    }
}

} // namespace sl::game::detail
