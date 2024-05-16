//
// Created by usatiynyan.
//

#include <sl/game.hpp>

#include <sl/meta/func/unit.hpp>

#include <libassert/assert.hpp>
#include <spdlog/spdlog.h>

namespace game = sl::game;

int main() {
    spdlog::set_level(spdlog::level::debug);

    game::graphics gfx =
        *ASSERT_VAL(game::graphics::initialize("00_blank_window", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f }));

    game::generic_input<bool(const sl::gfx::current_window&), 1> is_esc_pressed;
    auto detach_is_esc_pressed =
        is_esc_pressed.attach([](const sl::gfx::current_window& cw) { return cw.is_key_pressed(GLFW_KEY_ESCAPE); });

    while (!gfx.current_window.should_close()) {
        // input
        gfx.context->poll_events();
        gfx.state->frame_buffer_size.then([&cw = gfx.current_window](glm::ivec2 x) { cw.viewport(glm::ivec2{}, x); });

        if (is_esc_pressed(gfx.current_window).value_or(false)) {
            gfx.current_window.set_should_close(true);
        }

        // render
        gfx.current_window.clear(GL_COLOR_BUFFER_BIT);

        gfx.current_window.swap_buffers();
    }

    return 0;
}
