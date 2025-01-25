//
// Created by usatiynyan.
//

#include "sl/game/engine/layer.hpp"

namespace sl::game::engine {
namespace {

constexpr std::size_t default_string_capacity = 1024;
constexpr std::size_t default_shader_capacity = 16;
constexpr std::size_t default_vertex_capacity = 16;
constexpr std::size_t default_texture_capacity = 16;
constexpr std::size_t default_material_capacity = 16;

} // namespace

layer::layer(config cfg)
    : storage{ .string{ cfg.storage.string_capacity.value_or(default_string_capacity) },
               .shader{ cfg.storage.shader_capacity.value_or(default_shader_capacity) },
               .vertex{ cfg.storage.vertex_capacity.value_or(default_vertex_capacity) },
               .texture{ cfg.storage.texture_capacity.value_or(default_texture_capacity) },
               .material{ cfg.storage.material_capacity.value_or(default_material_capacity) } },
      loader{
          .shader{ storage.shader },
          .vertex{ storage.vertex },
          .texture{ storage.texture },
          .material{ storage.material },
      },
      root{ registry.create() } {}

} // namespace sl::game::engine
