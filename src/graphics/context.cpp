//
// Created by usatiynyan.
//

#include "sl/game/graphics/context.hpp"

namespace sl::game {

tl::optional<graphics> initialize_graphics(std::string_view title, glm::ivec2 size, glm::fvec4 clear_color) {
    constexpr gfx::context::options context_options{ 4, 6, GLFW_OPENGL_CORE_PROFILE };
    auto context = gfx::context::create(context_options);
    if (!context) {
        return tl::nullopt;
    }
    auto window = gfx::window::create(*context, title, size);
    if (!window) {
        return tl::nullopt;
    }
    auto current_window = window->make_current(*context, glm::ivec2{}, size, clear_color);
    std::unique_ptr<graphics_state> state{ new graphics_state{
        .frame_buffer_size{ size },
        .window_content_scale{ current_window.get_content_scale() },
    } };

    // this is safe, since callbacks will be called here only if window is alive
    (void)window->frame_buffer_size_cb.connect([state_ptr = state.get()](glm::ivec2 size) {
        state_ptr->frame_buffer_size.set(size);
    });

    (void)window->window_content_scale_cb.connect([state_ptr = state.get()](glm::fvec2 content_scale) {
        state_ptr->window_content_scale.set(content_scale);
    });

    return graphics{
        .context = std::move(context),
        .window = std::move(window),
        .current_window = std::move(current_window),
        .state = std::move(state),
        .imgui{ context_options, *window },
    };
}

} // namespace sl::game
