//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/context.hpp"

#include <sl/gfx/render.hpp>
#include <sl/meta/lifetime/finalizer.hpp>
#include <sl/meta/lifetime/unique.hpp>

namespace sl::game {

class bound_render;

struct render : public meta::unique {
    render(graphics& graphics, sl::gfx::projection projection);
    bound_render bind(sl::gfx::current_window& current_window, const graphics_state& graphics_state) const;

    sl::gfx::basis world;
    sl::gfx::camera camera;
};


class bound_render : public meta::finalizer<bound_render> {
public:
    explicit bound_render(
        const render& render,
        sl::gfx::current_window& current_window,
        const graphics_state& graphics_state
    );

public:
    glm::mat4 projection;
    glm::mat4 view;

private:
    std::reference_wrapper<sl::gfx::current_window> current_window_;
};


} // namespace sl::game
