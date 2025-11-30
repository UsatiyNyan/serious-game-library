//
// Created by usatiynyan.
//

#pragma once

#include "../common.hpp"

namespace script {

struct player_entity_state {
    bool q = false;
    bool w = false;
    bool e = false;
    bool a = false;
    bool s = false;
    bool d = false;
    sl::meta::dirty<bool> is_rotating;
    tl::optional<glm::dvec2> cursor_prev;
    glm::dvec2 cursor_curr;
};

inline exec::async<entt::entity> spawn_player_entity(game::engine::engine_context& e_ctx, game::engine::layer& layer) {
    const entt::entity entity = layer.registry.create();
    layer.registry.emplace<game::local_transform>(
        entity,
        game::transform{
            .tr{},
            .rot = glm::angleAxis(glm::radians(-180.0f), layer.world.up()),
        }
    );
    layer.registry.emplace<game::camera>(
        entity,
        game::perspective_projection{
            .fov = glm::radians(45.0f),
            .near = 0.1f,
            .far = 100.0f,
        }
    );

    layer.registry.emplace<player_entity_state>(entity);
    layer.registry.emplace<game::input<engine::layer>>(
        entity,
        [](engine::layer& layer, const game::input_events& input_events, entt::entity entity) {
            using action = game::keyboard_input_event::action_type;
            auto& state = *ASSERT_VAL((layer.registry.try_get<player_entity_state>(entity)));
            const sl::meta::pmatch handle{
                [&state](const game::keyboard_input_event& keyboard) {
                    using key = game::keyboard_input_event::key_type;
                    const bool is_down = keyboard.action == action::PRESS || keyboard.action == action::REPEAT;
                    switch (keyboard.key) {
                    case key::Q:
                        state.q = is_down;
                        break;
                    case key::W:
                        state.w = is_down;
                        break;
                    case key::E:
                        state.e = is_down;
                        break;
                    case key::A:
                        state.a = is_down;
                        break;
                    case key::S:
                        state.s = is_down;
                        break;
                    case key::D:
                        state.d = is_down;
                        break;
                    default:
                        break;
                    }
                },
                [&state](const game::mouse_button_input_event& mouse_button) {
                    using button = game::mouse_button_input_event::button_type;
                    if (mouse_button.button == button::RIGHT) {
                        state.is_rotating.set_if_ne(
                            mouse_button.action == action::PRESS || mouse_button.action == action::REPEAT
                        );
                    }
                },
                [&state](const game::cursor_input_event& cursor) {
                    // protection against multiple cursor events between updates
                    if (!state.cursor_prev.has_value()) {
                        state.cursor_prev.emplace(state.cursor_curr);
                    }
                    state.cursor_curr = cursor.pos;
                },
            };
            for (const auto& input_event : input_events) {
                handle(input_event);
            }
        }
    );
    layer.registry.emplace<game::update<engine::layer>>(
        entity,
        [&e_ctx](engine::layer& layer, entt::entity entity, game::time_point time_point) {
            if (!layer.registry.all_of<player_entity_state, game::transform>(entity)) {
                return;
            }

            auto& state = layer.registry.get<player_entity_state>(entity);
            auto& tf = layer.registry.get<game::transform>(entity);

            if (const sl::meta::defer consume{ [&state] { state.cursor_prev.reset(); } };
                state.cursor_prev.has_value() && state.is_rotating.get().value_or(false)) {
                const auto cursor_offset = state.cursor_curr - state.cursor_prev.value();

                constexpr float sensitivity = -glm::radians(0.2f);
                const float yaw = static_cast<float>(cursor_offset.x) * sensitivity;
                const float pitch = static_cast<float>(cursor_offset.y) * sensitivity;

                const glm::quat rot_yaw = glm::angleAxis(yaw, layer.world.y);
                tf.rotate(rot_yaw);

                const glm::vec3 camera_forward = tf.rot * layer.world.forward();
                const glm::vec3 camera_right = glm::cross(camera_forward, layer.world.up());
                const glm::quat rot_pitch = glm::angleAxis(pitch, camera_right);
                tf.rotate(rot_pitch);
            }

            {
                constexpr float acc = 5.0f;
                const float speed = acc * time_point.delta_sec().count();
                const auto new_tr = [&state, &world = layer.world] {
                    constexpr auto sub = [](bool a, bool b) {
                        return static_cast<float>(static_cast<int>(a) - static_cast<int>(b));
                    };
                    return sub(state.e, state.q) * world.up() + //
                           sub(state.d, state.a) * world.right() + //
                           sub(state.w, state.s) * world.forward();
                }();
                tf.translate(speed * (tf.rot * new_tr));
            }

            state.is_rotating.release().map([&e_ctx](bool is_rotating) {
                e_ctx.w_ctx.current_window.set_input_mode(
                    GLFW_CURSOR, is_rotating ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
                );
            });
        }
    );

    layer.registry.emplace<render::spot_light>(
        entity,
        render::spot_light{
            .ambient{ 0.0f, 0.0f, 0.0f },
            .diffuse{ 1.0f, 1.0f, 1.0f },
            .specular{ 1.0f, 1.0f, 1.0f },

            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f,

            .cutoff = glm::cos(glm::radians(12.5f)),
            .outer_cutoff = glm::cos(glm::radians(15.0f)),
        }
    );

    co_return entity;
}

} // namespace script
