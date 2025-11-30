//
// Created by usatiynyan.
//

#include <sl/game.hpp>

#include <libassert/assert.hpp>
#include <spdlog/spdlog.h>

namespace game = sl::game;

int main() {
    spdlog::set_level(spdlog::level::debug);

    auto gfx = *ASSERT_VAL(
        game::window_context::initialize(sl::meta::null, "00_blank_window", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f })
    );

    while (!gfx.current_window.should_close()) {
        // input
        gfx.context->poll_events();
        gfx.state->frame_buffer_size.release().map([&cw = gfx.current_window](const glm::ivec2& x) {
            cw.viewport(glm::ivec2{}, x);
        });

        if (gfx.current_window.is_key_pressed(GLFW_KEY_ESCAPE)) {
            gfx.current_window.set_should_close(true);
        }

        // render
        gfx.current_window.clear(GL_COLOR_BUFFER_BIT);

        gfx.current_window.swap_buffers();
    }

    return 0;
}
