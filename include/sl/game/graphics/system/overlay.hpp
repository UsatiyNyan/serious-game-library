//
// Created by usatiynyan.
//

#pragma once

#include <sl/ecs/layer.hpp>
#include <sl/gfx/ctx/imgui.hpp>

namespace sl::game {

struct overlay_system {
    void on_window_content_scale_changed(const glm::fvec2& window_content_scale) &;
    void execute(gfx::imgui_frame& imgui_frame) &;

public:
    ecs::layer& layer;
};

} // namespace sl::game
