//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/detail/log.hpp"
#include "sl/game/input/detail.hpp"

#include <glm/glm.hpp>
#include <libassert/assert.hpp>
#include <variant>
#include <vector>

namespace sl::game {

struct keyboard_input_event {
    using key_type = detail::keyboard_input_event;
    using action_type = detail::input_event_action;
    using mod_type = detail::input_event_mod;
    using mods_type = detail::input_event_mods;

    key_type key;
    action_type action;
    mods_type mods;
};

struct mouse_button_input_event {
    using button_type = detail::mouse_button_input_event;
    using action_type = detail::input_event_action;
    using mod_type = detail::input_event_mod;
    using mods_type = detail::input_event_mods;

    button_type button;
    action_type action;
    mods_type mods;
};

struct cursor_input_event {
    glm::dvec2 pos;
};

using input_event = std::variant<keyboard_input_event, mouse_button_input_event, cursor_input_event>;
using input_events = std::vector<input_event>;

// being careful with allocations here, want this to work super fast
class input_event_queue {
public:
    explicit input_event_queue(std::size_t capacity) { events_.reserve(capacity); }

    template <typename T>
    void push(T&& input_event) {
        const std::size_t prev_capacity = events_.capacity();
        events_.emplace_back(std::forward<T>(input_event));
        if (const std::size_t new_capacity = events_.capacity(); prev_capacity < new_capacity) {
            log::warn("[input_event_queue] reallocating: {} -> {}", prev_capacity, new_capacity);
        }
    }

    const auto& events() const { return events_; }

    void clear() {
        const std::size_t prev_capacity = events_.capacity();
        events_.clear(); // should not shrink the capacity
        ASSERT(prev_capacity == events_.capacity());
    }

private:
    std::vector<input_event> events_{};
};

} // namespace sl::game
