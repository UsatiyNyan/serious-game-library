//
// Created by usatiynyan.
//

#pragma once

#include <sl/gfx/ctx/context.hpp>
#include <sl/gfx/ctx/window.hpp>
#include <sl/gfx/ctx/imgui.hpp>
#include <sl/meta/conn/dirty.hpp>
#include <sl/meta/conn/scoped_conn.hpp>

#include <memory>
#include <tl/optional.hpp>

namespace sl::game {

struct graphics_state {
    meta::dirty<glm::ivec2> frame_buffer_size;
    meta::dirty<glm::fvec2> window_content_scale;
};

struct graphics {
    std::unique_ptr<gfx::context> context;
    std::unique_ptr<gfx::window> window;
    gfx::current_window current_window;
    std::unique_ptr<graphics_state> state;
    gfx::imgui_context imgui;
};

tl::optional<graphics> initialize_graphics(std::string_view title, glm::ivec2 size, glm::fvec4 clear_color);

} // namespace sl::game
