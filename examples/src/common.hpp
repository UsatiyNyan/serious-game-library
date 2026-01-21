//
// Created by usatiynyan.
//

#pragma once

#include <sl/ecs.hpp>
#include <sl/exec.hpp>
#include <sl/game.hpp>
#include <sl/gfx.hpp>
#include <sl/meta.hpp>
#include <sl/rt.hpp>

#include <imgui.h>
#include <stb/image.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <sl/meta/assert.hpp>
#include <range/v3/view.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

using sl::meta::operator""_us;
using sl::meta::operator""_ufs;

namespace sl {

struct VNT {
    gfx::va_attrib_field<3, float> vert;
    gfx::va_attrib_field<3, float> normal;
    gfx::va_attrib_field<2, float> tex_coords;
};

namespace script {

struct example_context {
    static example_context create(const game::engine_context& e_ctx) {
        // build/examples/ -> build/assets
        auto examples_path = e_ctx.root_path;
        auto build_path = examples_path.parent_path();
        auto asset_path = build_path / "assets";
        return example_context{
            .uss{ meta::unique_string_storage::init_type{} },
            .examples_path = std::move(examples_path),
            .build_path = std::move(build_path),
            .asset_path = std::move(asset_path),
        };
    }

public:
    meta::unique_string_storage uss;
    std::filesystem::path examples_path;
    std::filesystem::path build_path;
    std::filesystem::path asset_path;
};

inline exec::async<game::texture> create_texture(const std::filesystem::path& image_path, bool flip_vertically = true) {
    gfx::texture_builder tex_builder{ gfx::texture_type::texture_2d };
    tex_builder.set_wrap_s(gfx::texture_wrap::repeat);
    tex_builder.set_wrap_t(gfx::texture_wrap::repeat);
    tex_builder.set_min_filter(gfx::texture_filter::nearest);
    tex_builder.set_max_filter(gfx::texture_filter::nearest);

    const auto image = *ASSERT_VAL(stb::image_load(image_path, 4, flip_vertically));
    tex_builder.set_image(std::span{ image.dimensions }, gfx::texture_format{ GL_RGB, GL_RGBA }, image.data.get());
    co_return game::texture{ .tex = std::move(tex_builder).submit() };
}

template <typename VT, std::size_t vertices_extent, std::unsigned_integral indices_type, std::size_t indices_extent>
exec::async<game::vertex> create_vertex(
    std::span<const VT, vertices_extent> vertices,
    std::span<const indices_type, indices_extent> indices
) {
    gfx::vertex_array_builder va_builder;
    va_builder.attributes_from<VT>();
    auto vb = va_builder.buffer<gfx::buffer_type::array, gfx::buffer_usage::static_draw>(vertices);
    auto eb = va_builder.buffer<gfx::buffer_type::element_array, gfx::buffer_usage::static_draw>(indices);
    co_return game::vertex{
        .va = std::move(va_builder).submit(),
        .draw{ [vb = std::move(vb), eb = std::move(eb)](gfx::draw& draw) { draw.elements(eb); } },
    };
}

} // namespace script
} // namespace sl
