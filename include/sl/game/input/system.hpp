//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/input/component.hpp"
#include "sl/game/input/state.hpp"
#include "sl/game/layer.hpp"

namespace sl::game {

// TODO: maybe use stack<handle_result(...)> in order to create input priority system

template <typename Layer>
concept InputLayerRequirements = GameLayerRequirements<Layer>;

class input_system {
public:
    // attach to window
    explicit input_system(gfx::window& window)
        : key_conn_{ window.key_cb.connect([this](int key, int /* scancode */, int action, int mods) {
              // TODO: scancode?
              const bool is_pressed = action == GLFW_PRESS || action == GLFW_REPEAT;
              state_.keyboard.pressed[static_cast<std::size_t>(key)].set(is_pressed);
              state_.keyboard.mods.set(mods);
          }) },
          mouse_button_conn_{ window.mouse_button_cb.connect([this](int button, int action, int mods) {
              const bool is_pressed = action == GLFW_PRESS || action == GLFW_REPEAT;
              state_.mouse.pressed[static_cast<std::size_t>(button)].set(is_pressed);
              state_.mouse.mods.set(mods);
          }) },
          cursor_pos_conn_{ window.cursor_pos_cb.connect([this](glm::dvec2 cursor_pos) {
              state_.cursor.prev.set(state_.cursor.curr);
              state_.cursor.curr = cursor_pos;
          }) } {}

    const auto& state() const { return state_; }

    template <InputLayerRequirements Layer>
    void operator()(
        const gfx::basis& world,
        gfx::current_window& window,
        Layer& layer,
        const rt::time_point& time_point
    ) {
        auto entities = layer.registry.template view<input_component<Layer>>();
        for (auto&& [entity, input] : entities.each()) {
            input.handler(world, window, layer, entity, state_, time_point);
        }
    }

private:
    input_state state_{};

    meta::signal<int, int, int, int>::connection key_conn_;
    meta::signal<int, int, int>::connection mouse_button_conn_;
    meta::signal<glm::dvec2>::connection cursor_pos_conn_;
};

} // namespace sl::game
