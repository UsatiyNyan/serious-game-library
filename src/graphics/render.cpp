//
// Created by usatiynyan.
//

#include "sl/game/graphics/render.hpp"

namespace sl::game {

render::render(graphics& graphics, sl::gfx::projection projection)
    : camera{
          .proj = projection,
      } {
    graphics.current_window.enable(GL_DEPTH_TEST);
}

bound_render render::bind(sl::gfx::current_window& current_window, const graphics_state& graphics_state) const {
    return bound_render{ *this, current_window, graphics_state };
}

bound_render::bound_render(
    const render& render,
    sl::gfx::current_window& current_window,
    const graphics_state& graphics_state
)
    : finalizer{ [](bound_render& self) { self.current_window_.get().swap_buffers(); } },
      projection{ render.camera.projection(graphics_state.frame_buffer_size.get()) },
      view{ render.camera.view(render.world) }, current_window_{ current_window } {
    current_window.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

} // namespace sl::game
