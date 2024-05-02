//
// Created by usatiynyan.
//

#include "sl/game/graphics/context.hpp"

namespace sl::game {

tl::optional<graphics>
    initialize_graphics(std::string_view window_title, glm::ivec2 window_size, glm::fvec4 window_clear_color) {
    constexpr gfx::context::options context_options{ 4, 6, GLFW_OPENGL_CORE_PROFILE };
    auto context = gfx::context::create(context_options);
    if (!context) {
        return tl::nullopt;
    }
    auto window = gfx::window::create(*context, window_title, window_size);
    if (!window) {
        return tl::nullopt;
    }
    // this is safe, since callbacks will be called here only if window is alive
    (void)window->frame_buffer_size_cb.connect([window_ptr = window.get()](glm::ivec2 size) {
        gfx::current_window{ *window_ptr }.viewport(glm::ivec2{}, size);
    });
    auto current_window = window->make_current(*context, glm::ivec2{}, window_size, window_clear_color);
    return graphics{
        .context = std::move(context),
        .window = std::move(window),
        .current_window = std::move(current_window),
    };
}

} // namespace sl::game
