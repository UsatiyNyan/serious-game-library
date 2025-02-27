//
// Created by usatiynyan.
//

#pragma once

#include "../common.hpp"
#include <sl/exec/coro/async.hpp>

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
        .specular{ 0.0f, 1.0f, 0.0f },

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
                        const auto [maybe_tf, maybe_pl] =
                            layer.registry.try_get<game::transform, render::point_light>(entity);
                        if (!maybe_tf || !maybe_pl) {
                            continue;
                        }
                        const auto& tf = *maybe_tf;
                        const auto& pl = *maybe_pl;

                        const glm::mat4 transform =
                            camera_frame.projection * camera_frame.view * glm::translate(glm::mat4(1.0f), tf.tr);
                        set_transform(bound_sp, glm::value_ptr(transform));

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

inline exec::async<std::vector<entt::entity>>
    spawn_light_entities(engine::context& e_ctx, engine::layer& layer, const example_context& example_ctx) {
    const auto cube_vertex_id = "cube.vertex"_us(layer.storage.string);

    const auto unlit_shader_id = "unlit.shader"_us(layer.storage.string);
    std::ignore = co_await layer.loader.shader.emplace(unlit_shader_id, create_unlit_shader(example_ctx));

    std::vector<entt::entity> entities;

    for (const auto& [p, l] : ranges::views::zip(point_light_positions, point_lights)) {
        entt::entity entity = layer.registry.create();
        layer.registry.emplace<game::shader<engine::layer>::id>(entity, unlit_shader_id);
        layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
        layer.registry.emplace<game::local_transform>(entity, game::transform{ .tr = p });
        layer.registry.emplace<render::point_light>(entity, l);
        entities.push_back(entity);
    }

    std::vector<entt::entity> pl_entities = entities;

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

    exec::coro_schedule(
        *e_ctx.script_exec,
        [](engine::context& e_ctx, engine::layer& layer, std::vector<entt::entity> pl_entities) -> exec::async<void> {
            const auto initial_pls = [&] {
                std::vector<render::point_light> initial_pls;
                initial_pls.reserve(pl_entities.size());
                for (entt::entity pl_entity : pl_entities) {
                    initial_pls.push_back(layer.registry.get<render::point_light>(pl_entity));
                }
                return initial_pls;
            }();

            while (e_ctx.is_ok()) {
                const auto maybe_tp = co_await e_ctx.next_frame();
                if (!maybe_tp.has_value()) {
                    continue;
                }
                const auto& tp = maybe_tp.value();

                const float coef = std::sin(tp.delta_from_init_sec().count());
                for (const auto& [initial, pl_entity] : ranges::views::zip(initial_pls, pl_entities)) {
                    auto& current = layer.registry.get<render::point_light>(pl_entity);
                    current.ambient = initial.ambient * coef;
                    current.diffuse = initial.diffuse * coef;
                    current.specular = initial.specular * coef;
                }
            }
        }(e_ctx, layer, std::move(pl_entities))
    );

    co_return entities;
}

} // namespace script
