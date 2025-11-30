//
// Created by usatiynyan.
//

#pragma once

#include "../common.hpp"
#include "sl/game/graphics/buffer.hpp"

namespace script {

inline exec::async<game::shader> create_object_shader(const example_context& example_ctx, game::basis world) {
    const auto& root = example_ctx.asset_path;

    // load_from_file is potentially a blocking
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/blinn_phong.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/blinn_phong.frag")),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();

    constexpr std::array<std::string_view, 2> material_textures{
        "u_material.diffuse",
        "u_material.specular",
    };
    sp_bind.initialize_tex_units(std::span{ material_textures });

    auto set_view_pos = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_view_pos"));

    auto set_directional_light_size =
        *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1ui, "u_directional_light_size"));
    auto set_point_light_size = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1ui, "u_point_light_size"));
    auto set_spot_light_size = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1ui, "u_spot_light_size"));

    auto set_material_diffuse_color =
        *ASSERT_VAL(sp_bind.make_uniform_v_setter(glUniform4fv, "u_material.diffuse_color", 1));
    auto set_material_specular_color =
        *ASSERT_VAL(sp_bind.make_uniform_v_setter(glUniform4fv, "u_material.specular_color", 1));
    auto set_material_mode = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1ui, "u_material.mode"));
    auto set_material_shininess = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1f, "u_material.shininess"));

    auto set_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_model", 1, false));
    auto set_it_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix3fv, "u_it_model", 1, false));
    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));

    auto dl_buffer = game::make_and_initialize_ssbo<game::render::directional_light_element>(1);
    auto pl_buffer = game::make_and_initialize_ssbo<game::render::point_light_element>(16);
    auto sl_buffer = game::make_and_initialize_ssbo<game::render::spot_light_element>(1);

    co_return game::shader{
        .sp{ std::move(sp) },
        .setup{ [ //
                    world,
                    set_view_pos = std::move(set_view_pos),
                    dl_buffer = std::move(dl_buffer),
                    set_dl_size = std::move(set_directional_light_size),
                    pl_buffer = std::move(pl_buffer),
                    set_pl_size = std::move(set_point_light_size),
                    sl_buffer = std::move(sl_buffer),
                    set_sl_size = std::move(set_spot_light_size),
                    set_material_diffuse_color = std::move(set_material_diffuse_color),
                    set_material_specular_color = std::move(set_material_specular_color),
                    set_material_mode = std::move(set_material_mode),
                    set_material_shininess = std::move(set_material_shininess),
                    set_model = std::move(set_model),
                    set_it_model = std::move(set_it_model),
                    set_transform = std::move(set_transform)](
                    const ecs::layer& layer,
                    const game::camera_frame& camera_frame,
                    const gfx::bound_shader_program& bound_sp
                ) mutable {
            set_view_pos(bound_sp, camera_frame.position.x, camera_frame.position.y, camera_frame.position.z);

            const std::uint32_t dl_size = game::fill_ssbo(layer, world, dl_buffer);
            set_dl_size(bound_sp, dl_size);

            const std::uint32_t pl_size = game::fill_ssbo(layer, world, pl_buffer);
            set_pl_size(bound_sp, pl_size);

            const std::uint32_t sl_size = game::fill_ssbo(layer, world, sl_buffer);
            set_sl_size(bound_sp, sl_size);

            return [ //
                       &set_material_shininess,
                       &set_model,
                       &set_it_model,
                       &set_transform,
                       &set_material_diffuse_color,
                       &set_material_specular_color,
                       &set_material_mode,
                       &layer,
                       &camera_frame,
                       &bound_sp,
                       bound_dl_base = dl_buffer.bind_base(0),
                       bound_pl_base = pl_buffer.bind_base(1),
                       bound_sl_base = sl_buffer.bind_base(2)]( //
                       const gfx::bound_vertex_array& bound_va,
                       game::vertex::draw_type& vertex_draw,
                       std::span<const entt::entity> entities
                   ) {
                for (const entt::entity entity : entities) {
                    const auto [maybe_tf, maybe_mtl_id] =
                        layer.registry.try_get<game::transform, game::material::id>(entity);
                    if (!maybe_tf || !maybe_mtl_id) {
                        game::log::warn("entity={} missing transform or material", static_cast<std::uint32_t>(entity));
                        continue;
                    }
                    const auto& tf = *maybe_tf;
                    const auto& mtl_id = *maybe_mtl_id;

                    const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), tf.tr);
                    const glm::mat4 rotation_matrix = glm::mat4_cast(tf.rot);
                    // TODO: scale
                    const glm::mat4 model = translation_matrix * rotation_matrix;
                    const glm::mat3 it_model = glm::transpose(glm::inverse(model));
                    const glm::mat4 transform = camera_frame.projection * camera_frame.view * model;
                    set_model(bound_sp, glm::value_ptr(model));
                    set_it_model(bound_sp, glm::value_ptr(it_model));
                    set_transform(bound_sp, glm::value_ptr(transform));

                    const auto mtl = *ASSERT_VAL(layer.storage.material.lookup(mtl_id.id));
                    const sl::meta::maybe<gfx::bound_texture> maybe_bound_diffuse_tex =
                        mtl->diffuse
                        | sl::meta::pmatch{
                              [](const sl::meta::persistent<game::texture>& tex
                              ) -> sl::meta::maybe<gfx::bound_texture> { return tex->tex.activate(0); },
                              [&](const glm::vec4& clr) -> sl::meta::maybe<gfx::bound_texture> {
                                  set_material_diffuse_color(bound_sp, glm::value_ptr(clr));
                                  return sl::meta::null;
                              },
                          };
                    const sl::meta::maybe<gfx::bound_texture> maybe_bound_specular_tex =
                        mtl->specular
                        | sl::meta::pmatch{
                              [](const sl::meta::persistent<game::texture>& tex
                              ) -> sl::meta::maybe<gfx::bound_texture> { return tex->tex.activate(1); },
                              [&](const glm::vec4& clr) -> sl::meta::maybe<gfx::bound_texture> {
                                  set_material_specular_color(bound_sp, glm::value_ptr(clr));
                                  return sl::meta::null;
                              },
                          };
                    const std::uint32_t mtl_mode = (maybe_bound_diffuse_tex.has_value() ? 0b01 : 0)
                                                   + (maybe_bound_specular_tex.has_value() ? 0b10 : 0);
                    set_material_mode(bound_sp, mtl_mode);

                    set_material_shininess(bound_sp, mtl->shininess);

                    gfx::draw draw{ bound_sp, bound_va };
                    vertex_draw(draw);
                }
            };
        } },
    };
}

inline exec::async<game::material> create_crate_material(engine::layer& layer, const example_context& example_ctx) {
    const auto crate_texture_diffuse_id = "crate.texture.diffuse"_us(layer.storage.string);
    const auto crate_texture_specular_id = "crate.texture.specular"_us(layer.storage.string);

    const auto crate_texture_diffuse = co_await layer.loader.texture.emplace(
        crate_texture_diffuse_id, create_texture(example_ctx.examples_path / "textures/03_lightmap_diffuse.png")
    );
    const auto crate_texture_specular = co_await layer.loader.texture.emplace(
        crate_texture_specular_id, create_texture(example_ctx.examples_path / "textures/03_lightmap_specular.png")
    );

    co_return game::material{
        .diffuse = crate_texture_diffuse ? crate_texture_diffuse.value() : crate_texture_diffuse.error(),
        .specular = crate_texture_specular ? crate_texture_specular.value() : crate_texture_specular.error(),
        .shininess = 128.0f * 0.6f,
    };
}

inline exec::async<std::vector<entt::entity>> spawn_box_entities(
    engine::engine_context& e_ctx [[maybe_unused]],
    engine::layer& layer,
    const example_context& example_ctx
) {
    // basically require this id to be loaded
    const auto cube_vertex_id = "cube.vertex"_us(layer.storage.string);
    std::ignore = co_await layer.loader.vertex.get(cube_vertex_id);

    const auto object_shader_id = "object.shader"_us(layer.storage.string);
    std::ignore = co_await layer.loader.shader.emplace(object_shader_id, create_object_shader(example_ctx));

    const auto crate_material_id = "crate.material"_us(layer.storage.string);
    std::ignore = co_await layer.loader.material.emplace(crate_material_id, create_crate_material(layer, example_ctx));

    const auto solid_material_id = "solid.material"_us(layer.storage.string);
    std::ignore = co_await layer.loader.material.emplace(solid_material_id, []() -> exec::async<game::material> {
        co_return game::material{
            .diffuse = glm::vec4{ 1.0f },
            .specular = glm::vec4{ 0.0f },
            .shininess = 128.0f * 0.6f,
        };
    }());

    constexpr auto generate_positions = [](std::size_t rows, std::size_t cols) -> exec::generator<glm::vec3> {
        for (std::size_t i = 0; i < rows; ++i) {
            for (std::size_t j = 0; j < cols; ++j) {
                co_yield glm::vec3{
                    static_cast<float>(i) * 2.0f,
                    0.0f,
                    static_cast<float>(j) * 2.0f,
                };
            }
        }
    };
    constexpr std::size_t rows = 5;
    constexpr std::size_t cols = 5;

    std::vector<entt::entity> entities;
    entities.reserve(rows * cols);

    auto generator = generate_positions(rows, cols);
    for (const auto [index, position] : ranges::views::enumerate(generator)) {
        entt::entity entity = layer.registry.create();

        const float angle0 = 20.0f * (static_cast<float>(index));
        layer.registry.emplace<game::shader<engine::layer>::id>(entity, object_shader_id);
        layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
        layer.registry.emplace<game::material::id>(entity, crate_material_id);
        layer.registry.emplace<game::local_transform>(
            entity,
            game::transform{
                .tr = position,
                .rot = glm::angleAxis(glm::radians(angle0), layer.world.up()),
            }
        );
        layer.registry.emplace<game::update<engine::layer>>(
            entity,
            [angle0](engine::layer& layer, entt::entity entity, game::time_point tp) {
                const float t = tp.now_sec().count();
                const float angle_v = 2.f;
                const float angle = angle0 + (angle_v * t);
                auto& local_tf = layer.registry.get<game::local_transform>(entity);
                local_tf.set(game::transform{
                    .tr = local_tf.get()->tr,
                    .rot = glm::angleAxis(glm::radians(angle), layer.world.up()),
                });
            }
        );

        entities.push_back(entity);
    }

    if (!entities.empty()) {
        entt::entity entity = layer.registry.create();

        layer.registry.emplace<game::shader<engine::layer>::id>(entity, object_shader_id);
        layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
        layer.registry.emplace<game::material::id>(entity, solid_material_id);
        layer.registry.emplace<game::local_transform>(entity, game::transform{ .tr{ 0.0f, 2.0f, 0.0f } });

        entities.push_back(entity);
    }
    co_return entities;
}

} // namespace script
