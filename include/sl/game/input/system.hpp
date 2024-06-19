//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/input/component.hpp"
#include "sl/game/input/event.hpp"
#include "sl/game/layer.hpp"

namespace sl::game {

// TODO: maybe use stack<handle_result(...)> in order to create input priority system

template <typename Layer>
concept InputLayerRequirements = GameLayerRequirements<Layer>;

class input_system {
public:
    // attach to window
    explicit input_system(gfx::window& window)
        : queue_{ 128 }, //
          key_conn_{ window.key_cb.connect([this](int key, int /* scancode */, int action, int mods) {
              // TODO: scancode?
              queue_.push(keyboard_input_event{
                  .key{ static_cast<std::uint8_t>(key) },
                  .action{ static_cast<std::uint8_t>(action) },
                  .mods{ static_cast<std::uint8_t>(mods) },
              });
          }) },
          mouse_button_conn_{ window.mouse_button_cb.connect([this](int button, int action, int mods) {
              queue_.push(mouse_button_input_event{
                  .button{ static_cast<std::uint8_t>(button) },
                  .action{ static_cast<std::uint8_t>(action) },
                  .mods{ static_cast<std::uint8_t>(mods) },
              });
          }) },
          cursor_pos_conn_{ window.cursor_pos_cb.connect([this](glm::dvec2 cursor_pos) {
              queue_.push(cursor_input_event{ .pos = cursor_pos });
          }) } {}

    template <InputLayerRequirements Layer>
    void operator()(Layer& layer) {
        auto entities = layer.registry.template view<input_component<Layer>>();
        for (auto&& [entity, input] : entities.each()) {
            input.handler(layer, entity, queue_.events());
        }
    }

private:
    input_event_queue queue_;

    meta::signal<int, int, int, int>::connection key_conn_;
    meta::signal<int, int, int>::connection mouse_button_conn_;
    meta::signal<glm::dvec2>::connection cursor_pos_conn_;
};

} // namespace sl::game
