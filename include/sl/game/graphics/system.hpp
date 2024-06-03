//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/layer.hpp"
#include <sl/rt/time.hpp>

namespace sl::game {

class graphics_system {
public:
    void render(layer& layer, rt::time_point time_point);

private:
};

} // namespace sl::game
