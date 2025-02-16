//
// Created by usatiynyan.
//

#pragma once

#include "../common.hpp"

namespace script {

constexpr std::array point_light_positions{
    glm::vec3{ 10.0f, 0.0f, 0.0f },
    glm::vec3{ 0.0f, 10.0f, 0.0f },
    glm::vec3{ 0.0f, 0.0f, 10.0f },
};

constexpr std::array point_lights{
    render::point_light{
        .ambient{ 1.0f, 0.0f, 0.0f },
        .diffuse{ 1.0f, 0.0f, 0.0f },
        .specular{ 1.0f, 0.0f, 0.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
    render::point_light{
        .ambient{ 0.0f, 1.0f, 0.0f },
        .diffuse{ 0.0f, 1.0f, 0.0f },
        .specular{ 1.0f, 1.0f, 1.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
    render::point_light{
        .ambient{ 0.0f, 0.0f, 1.0f },
        .diffuse{ 0.0f, 0.0f, 1.0f },
        .specular{ 0.0f, 0.0f, 1.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
};

inline exec::async<game::shader<engine::layer>> create_unlit_shader(const example_context& example_ctx) {
    const auto& root = example_ctx.asset_path;

    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/unlit.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/unlit.frag")),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();
    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));
    auto set_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_color"));

    co_return game::shader<engine::layer>{
        .sp = std::move(sp),
        .setup{
            [ //
                set_transform = std::move(set_transform),
                set_color = std::move(set_color)]( //
                engine::layer& layer,
                const game::camera_frame& camera_frame,
                const gfx::bound_shader_program& bound_sp
            ) {
                return [ //
                           &set_transform,
                           &set_color,
                           &layer,
                           &camera_frame,
                           &bound_sp]( //
                           const gfx::bound_vertex_array& bound_va,
                           game::vertex::draw_type& vertex_draw,
                           std::span<const entt::entity> entities
                       ) {
                    for (const entt::entity entity : entities) {
                        if (const bool has_components =
                                layer.registry.all_of<game::transform, render::point_light>(entity);
                            !ASSUME_VAL(has_components)) {
                            continue;
                        }

                        const auto& tf = layer.registry.get<game::transform>(entity);
                        const glm::mat4 transform =
                            camera_frame.projection * camera_frame.view * glm::translate(glm::mat4(1.0f), tf.tr);
                        set_transform(bound_sp, glm::value_ptr(transform));

                        const auto& pl = layer.registry.get<render::point_light>(entity);
                        const glm::vec3& color = pl.ambient;
                        set_color(bound_sp, color.r, color.g, color.b);

                        gfx::draw draw{ bound_sp, bound_va };
                        vertex_draw(draw);
                    }
                };
            },
        },
    };
}

inline exec::async<std::vector<entt::entity>> spawn_light_entities(
    engine::context& e_ctx [[maybe_unused]],
    engine::layer& layer,
    const example_context& example_ctx
) {
    const auto cube_vertex_id = "cube.vertex"_us(layer.storage.string);

    const auto unlit_shader_id = "unlit.shader"_us(layer.storage.string);
    std::ignore = co_await layer.loader.shader.emplace(unlit_shader_id, create_unlit_shader(example_ctx));

    std::vector<entt::entity> entities;

    for (const auto& [p, l] : ranges::views::zip(point_light_positions, point_lights)) {
        entt::entity entity = layer.registry.create();
        layer.registry.emplace<game::shader<engine::layer>::id>(entity, unlit_shader_id);
        layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
        layer.registry.emplace<game::local_transform>(entity, game::transform{ .tr = p, .rot{} });
        layer.registry.emplace<render::point_light>(entity, l);
        entities.push_back(entity);
    }

    {
        entt::entity entity = layer.registry.create();
        auto rot = glm::rotation(layer.world.forward(), glm::normalize(glm::vec3{ -0.2f, -1.0f, -0.3f }));
        layer.registry.emplace<game::local_transform>(entity, game::transform{ .tr{}, .rot{ rot } });
        layer.registry.emplace<render::directional_light>(
            entity,
            render::directional_light{
                .ambient{ 0.05f, 0.05f, 0.05f },
                .diffuse{ 0.4f, 0.4f, 0.4f },
                .specular{ 0.5f, 0.5f, 0.5f },
            }
        );
        entities.push_back(entity);
    }

    co_return entities;
}

} // namespace script
