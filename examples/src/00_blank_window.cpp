//
// Created by usatiynyan.
//

#include "sl/game.hpp"

#include <libassert/assert.hpp>
#include <spdlog/spdlog.h>

namespace game = sl::game;

int main() {
    spdlog::set_level(spdlog::level::debug);

    game::graphics gfx =
        *ASSERT_VAL(game::initialize_graphics("00_blank_window", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f }));

    game::generic_control<void(sl::gfx::current_window&), 1> is_key_pressed;
    const auto detach_is_key_pressed = is_key_pressed.attach([](sl::gfx::current_window& cw) {
        if (cw.is_key_pressed(GLFW_KEY_ESCAPE)) {
            cw.set_should_close(true);
        }
    });

    while (!gfx.current_window.should_close()) {
        // control
        gfx.context->poll_events();
        is_key_pressed(gfx.current_window);

        // render
        gfx.current_window.clear(GL_COLOR_BUFFER_BIT);

        gfx.current_window.swap_buffers();
    }

    return 0;
}
