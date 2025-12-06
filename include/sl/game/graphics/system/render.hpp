//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/basis.hpp"
#include "sl/game/graphics/context.hpp"

#include <sl/ecs/layer.hpp>

#include <sl/meta/monad/result.hpp>
#include <sl/meta/type/unit.hpp>

namespace sl::game {

struct graphics_system {
    enum class error_type : std::uint8_t {
        NO_SHADER_STORAGE,
        NO_VERTEX_STORAGE,
    };

public:
    // TODO: framebuffer as a "return value".
    meta::result<meta::unit, error_type> execute(const window_frame& a_window_frame) &;

public:
    ecs::layer& layer;
    basis world;
};

} // namespace sl::game
