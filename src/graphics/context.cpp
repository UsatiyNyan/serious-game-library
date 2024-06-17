//
// Created by usatiynyan.
//

#include "sl/game/graphics/context.hpp"

namespace sl::game {

tl::optional<graphics> graphics::initialize(std::string_view title, glm::ivec2 size, glm::fvec4 clear_color) {
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

    gfx::imgui_context imgui{ context_options, *window };

    return graphics{
        .context = std::move(context),
        .window = std::move(window),
        .current_window = std::move(current_window),
        .state = std::move(state),
        .imgui = std::move(imgui),
    };
}

graphics_frame::graphics_frame(gfx::current_window& current_window) : current_window_{ current_window } {
    current_window_.enable(GL_DEPTH_TEST);
}

meta::defer<fu2::capacity_default> graphics_frame::begin() {
    current_window_.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return meta::defer<fu2::capacity_default>{ [&cw = current_window_] { cw.swap_buffers(); } };
}

} // namespace sl::game
