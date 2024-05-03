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

bound_render render::bind(sl::gfx::current_window& current_window) const {
    return bound_render{ *this, current_window };
}

bound_render::bound_render(const render&, sl::gfx::current_window& current_window)
    : finalizer{ [](bound_render& self) { self.current_window_.get().swap_buffers(); } },
      current_window_{ current_window } {
    current_window.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

} // namespace sl::game
