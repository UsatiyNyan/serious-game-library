//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/input/component.hpp"
#include "sl/game/input/detail.hpp"
#include "sl/game/input/event.hpp"
#include "sl/game/layer.hpp"

namespace sl::game {

// TODO: maybe use stack<handle_result(...)> in order to create input priority system

template <typename Layer>
concept InputLayerRequirements = GameLayerRequirements<Layer>;

class input_system : meta::immovable {
public:
    // attach to window
    explicit input_system(gfx::window& window, std::size_t queue_capacity = 128)
        : queue_{ queue_capacity }, //
          key_conn_{ window.key_cb.connect([this](int key, int /* scancode */, int action, int mods) {
              // TODO: scancode?
              queue_.push(keyboard_input_event{
                  .key = detail::keyboard_input_event_from_glfw(key),
                  .action = detail::input_event_action_from_glfw(action),
                  .mods = detail::input_event_mods_from_glfw(mods),
              });
          }) },
          mouse_button_conn_{ window.mouse_button_cb.connect([this](int button, int action, int mods) {
              queue_.push(mouse_button_input_event{
                  .button = detail::mouse_button_input_event_from_glfw(button),
                  .action = detail::input_event_action_from_glfw(action),
                  .mods = detail::input_event_mods_from_glfw(mods),
              });
          }) },
          cursor_pos_conn_{ window.cursor_pos_cb.connect([this](glm::dvec2 cursor_pos) {
              queue_.push(cursor_input_event{ .pos = cursor_pos });
          }) } {}

    template <InputLayerRequirements Layer>
    void process(Layer& layer) {
        auto entities = layer.registry.template view<input<Layer>>();
        for (auto&& [entity, input] : entities.each()) {
            input.handler(layer, queue_.events(), entity);
        }
        queue_.clear();
    }

private:
    input_event_queue queue_;

    meta::signal<int, int, int, int>::connection key_conn_;
    meta::signal<int, int, int>::connection mouse_button_conn_;
    meta::signal<glm::dvec2>::connection cursor_pos_conn_;
};

} // namespace sl::game
