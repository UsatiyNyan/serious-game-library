//
// Created by usatiynyan.
//

#pragma once

#include <sl/gfx/ctx/context.hpp>
#include <sl/gfx/ctx/window.hpp>

#include <tl/optional.hpp>

namespace sl::game {

struct graphics {
    std::unique_ptr<gfx::context> context;
    std::unique_ptr<gfx::window> window;
    gfx::current_window current_window;
};

tl::optional<graphics>
    initialize_graphics(std::string_view window_title, glm::ivec2 window_size, glm::fvec4 window_clear_color);

} // namespace sl::game
