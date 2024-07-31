//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/basis.hpp"
#include "sl/game/graphics/component/camera.hpp"

#include <sl/gfx/ctx/context.hpp>
#include <sl/gfx/ctx/imgui.hpp>
#include <sl/gfx/ctx/window.hpp>

#include <sl/meta/conn/dirty.hpp>
#include <sl/meta/conn/scoped_conn.hpp>
#include <sl/meta/lifetime/defer.hpp>
#include <sl/meta/lifetime/finalizer.hpp>

#include <memory>
#include <tl/optional.hpp>

namespace sl::game {

class window_frame;

struct window_state {
    meta::dirty<glm::ivec2> frame_buffer_size;
    meta::dirty<glm::fvec2> window_content_scale;
};

struct window_context {
    [[nodiscard]] static tl::optional<window_context>
        initialize(std::string_view title, glm::ivec2 size, glm::fvec4 clear_color);
    [[nodiscard]] window_frame new_frame();

public:
    std::unique_ptr<gfx::context> context;
    std::unique_ptr<gfx::window> window;
    gfx::current_window current_window;
    std::unique_ptr<window_state> state;
    gfx::imgui_context imgui;

    // TODO: is there a better place for that?
    static constexpr basis world;
};

struct camera_frame {
    basis world;
    glm::vec3 position;
    glm::mat4 projection;
    glm::mat4 view;
};

class window_frame : public meta::finalizer<window_frame> {
public:
    explicit window_frame(window_context& ctx);

    [[nodiscard]] camera_frame for_camera(const camera& camera, const transform& tf) const;

private:
    window_context& ctx_;
};

} // namespace sl::game
