//
// Created by usatiynyan.
//

#include "sl/game/graphics/system/overlay.hpp"
#include "sl/game/graphics/component/overlay.hpp"

namespace sl::game {

void overlay_system::on_window_content_scale_changed(const glm::fvec2& window_content_scale) & {
    ImGui::GetStyle().ScaleAllSizes(window_content_scale.x);
}

void overlay_system::execute(gfx::imgui_frame& imgui_frame) & {
    for (auto [entity, an_overlay] : layer.registry.view<overlay>().each()) {
        an_overlay(layer, imgui_frame, entity);
    }
}

} // namespace sl::game
