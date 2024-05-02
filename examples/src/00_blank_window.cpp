//
// Created by usatiynyan.
//

#include "sl/game.hpp"

#include <libassert/assert.hpp>
#include <spdlog/spdlog.h>

namespace game = sl::game;

int main() {
    spdlog::set_level(spdlog::level::debug);

    game::graphics gfx = *ASSERT_VAL(game::initialize_graphics("00_blank_window", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f }));

    while (!gfx.current_window.should_close()) {
        // control
        gfx.context->poll_events();

        if (gfx.current_window.is_key_pressed(GLFW_KEY_ESCAPE)) {
            gfx.current_window.set_should_close(true);
        }

        // render
        gfx.current_window.clear(GL_COLOR_BUFFER_BIT);

        gfx.current_window.swap_buffers();
    }

    return 0;
}
