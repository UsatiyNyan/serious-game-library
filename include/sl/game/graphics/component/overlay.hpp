//
// Created by usatiynyan.
//

#pragma once

#include <sl/ecs/layer.hpp>

#include <sl/gfx/ctx/imgui.hpp>
#include <sl/meta/func/function.hpp>

namespace sl::game {

using overlay = meta::unique_function<void(ecs::layer&, gfx::imgui_frame&, entt::entity)>;

} // namespace sl::game
