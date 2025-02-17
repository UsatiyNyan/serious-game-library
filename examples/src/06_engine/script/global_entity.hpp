//
// Created by usatiynyan.
//

#pragma once

#include "../common.hpp"

namespace script {

struct global_entity_state {
    sl::meta::dirty<bool> should_close;
};

inline exec::async<entt::entity> spawn_global_entity(game::engine::context& e_ctx, game::engine::layer& layer) {
    const entt::entity entity = layer.root;

    layer.registry.emplace<global_entity_state>(entity);
    layer.registry.emplace<game::input<engine::layer>>(
        entity,
        [](engine::layer& layer, const game::input_events& input_events, entt::entity entity) {
            using action = game::keyboard_input_event::action_type;
            auto& state = *ASSERT_VAL((layer.registry.try_get<global_entity_state>(entity)));
            const sl::meta::pmatch handle{
                [&state](const game::keyboard_input_event& keyboard) {
                    using key = game::keyboard_input_event::key_type;
                    if (keyboard.key == key::ESCAPE) {
                        const bool prev = state.should_close.get().value_or(false);
                        state.should_close.set(prev || keyboard.action == action::PRESS);
                    }
                },
                [](const auto&) {},
            };
            for (const auto& input_event : input_events) {
                handle(input_event);
            }
        }
    );
    layer.registry.emplace<game::update<engine::layer>>(
        entity,
        [&](engine::layer& layer, entt::entity entity, game::time_point) {
            auto& state = *ASSERT_VAL((layer.registry.try_get<global_entity_state>(entity)));
            state.should_close.release().map([&cw = e_ctx.w_ctx.current_window](bool should_close) {
                cw.set_should_close(should_close);
            });
        }
    );
    layer.registry.emplace<game::local_transform>(entity);
    layer.registry.emplace<game::transform>(entity);

    co_return entity;
}

} // namespace script
