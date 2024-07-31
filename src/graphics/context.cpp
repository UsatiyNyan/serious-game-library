//
// Created by usatiynyan.
//

#include "sl/game/graphics/context.hpp"

namespace sl::game {

tl::optional<window_context>
    window_context::initialize(std::string_view title, glm::ivec2 size, glm::fvec4 clear_color) {
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
    current_window.enable(GL_DEPTH_TEST); // TODO: sync with clear in graphics_frame, store behaviour

    std::unique_ptr<window_state> state{ new window_state{
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

    return window_context{
        .context = std::move(context),
        .window = std::move(window),
        .current_window = std::move(current_window),
        .state = std::move(state),
        .imgui = std::move(imgui),
    };
}

window_frame window_context::new_frame() { return window_frame{ *this }; }

window_frame::window_frame(window_context& ctx)
    : finalizer{ [](window_frame& self) { self.ctx_.current_window.swap_buffers(); } }, ctx_{ ctx } {
    ctx_.current_window.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

camera_frame window_frame::for_camera(const camera& camera, const transform& tf) const {
    return camera_frame{
        .world = ctx_.world,
        .position = tf.tr,
        .projection = camera.calculate_projection(ctx_.state->frame_buffer_size.get()),
        .view = ctx_.world.view(tf),
    };
}

} // namespace sl::game
