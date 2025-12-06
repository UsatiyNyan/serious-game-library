//
// Created by usatiynyan.
//

#include "common.hpp"
#include "sl/game/graphics/component/overlay.hpp"
#include "sl/game/graphics/component/transform.hpp"
#include "sl/game/graphics/system/overlay.hpp"
#include "sl/game/graphics/system/render.hpp"

namespace sl {

struct global_entity_state {
    meta::dirty<bool> should_close;
};

struct source_entity_state {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct cursor_state {
    glm::dvec2 first{};
    glm::dvec2 last{};
};

struct player_entity_state {
    meta::maybe<cursor_state> cursor;

    meta::dirty<bool> rmb{ false };

    bool esc = false;
    bool q = false;
    bool w = false;
    bool e = false;
    bool a = false;
    bool s = false;
    bool d = false;
};

struct material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

struct VN {
    gfx::va_attrib_field<3, float> vert;
    gfx::va_attrib_field<3, float> normal;
};

exec::async<game::shader> create_source_shader(const script::example_context& example_ctx) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(
            gfx::shader_type::vertex, example_ctx.examples_path / "shaders/01_lighting_source.vert"
        )),
        *ASSERT_VAL(gfx::shader::load_from_file(
            gfx::shader_type::fragment, example_ctx.examples_path / "shaders/01_lighting_source.frag"
        )),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();

    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));
    auto set_light_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light_color"));
    co_return game::shader{
        .sp{ std::move(sp) },
        .setup{ [ //
                    set_transform = std::move(set_transform),
                    set_light_color = std::move(set_light_color)](
                    const ecs::layer& layer, //
                    const game::camera_frame& camera_frame,
                    const gfx::bound_shader_program& bound_sp
                ) mutable {
            return [&](const gfx::bound_vertex_array& bound_va,
                       game::vertex::draw_type& vertex_draw,
                       std::span<const entt::entity> entities) {
                gfx::draw draw{ bound_sp, bound_va };

                for (const entt::entity entity : entities) {
                    const auto [state, tf] = layer.registry.try_get<source_entity_state, game::transform>(entity);

                    if (state) {
                        set_light_color(bound_sp, state->ambient.r, state->ambient.g, state->ambient.b);
                    }

                    if (tf) {
                        const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), tf->tr);
                        const glm::mat4 rotation_matrix = glm::mat4_cast(tf->rot);
                        const glm::mat4 model = translation_matrix * rotation_matrix;
                        const glm::mat4 transform = camera_frame.projection * camera_frame.view * model;
                        set_transform(bound_sp, glm::value_ptr(transform));
                    }

                    vertex_draw(draw);
                }
            };
        } },
    };
}

exec::async<game::shader> create_object_shader(const script::example_context& example_ctx) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(
            gfx::shader_type::vertex, example_ctx.examples_path / "shaders/02_material_object.vert"
        )),
        *ASSERT_VAL(gfx::shader::load_from_file(
            gfx::shader_type::fragment, example_ctx.examples_path / "shaders/02_material_object.frag"
        )),
    };

    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();

    auto set_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_model", 1, false));
    auto set_it_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix3fv, "u_it_model", 1, false));
    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));

    auto set_view_pos = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_view_pos"));

    auto set_light_position = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.position"));
    auto set_light_ambient = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.ambient"));
    auto set_light_diffuse = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.diffuse"));
    auto set_light_specular = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.specular"));

    auto set_material_ambient = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_material.ambient"));
    auto set_material_diffuse = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_material.diffuse"));
    auto set_material_specular = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_material.specular"));
    auto set_material_shininess = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1f, "u_material.shininess"));

    co_return game::shader{
        .sp{ std::move(sp) },
        .setup{ [ //
                    set_model = std::move(set_model),
                    set_it_model = std::move(set_it_model),
                    set_transform = std::move(set_transform),
                    set_view_pos = std::move(set_view_pos),
                    set_light_position = std::move(set_light_position),
                    set_light_ambient = std::move(set_light_ambient),
                    set_light_diffuse = std::move(set_light_diffuse),
                    set_light_specular = std::move(set_light_specular),
                    set_material_ambient = std::move(set_material_ambient),
                    set_material_diffuse = std::move(set_material_diffuse),
                    set_material_specular = std::move(set_material_specular),
                    set_material_shininess = std::move(set_material_shininess)](
                    const ecs::layer& layer,
                    const game::camera_frame& camera_frame,
                    const gfx::bound_shader_program& bound_sp
                ) mutable {
            {
                const auto& tr = camera_frame.position;
                set_view_pos(bound_sp, tr.x, tr.y, tr.z);
            }

            for (auto [source_entity, source_state, tf] :
                 layer.registry.view<source_entity_state, game::transform>().each()) {
                set_light_position(bound_sp, tf.tr.x, tf.tr.y, tf.tr.z);
                set_light_ambient(bound_sp, source_state.ambient.r, source_state.ambient.g, source_state.ambient.b);
                set_light_diffuse(bound_sp, source_state.diffuse.r, source_state.diffuse.g, source_state.diffuse.b);
                set_light_specular(bound_sp, source_state.specular.r, source_state.specular.g, source_state.specular.b);
            }

            return [&](const gfx::bound_vertex_array& bound_va,
                       game::vertex::draw_type& vertex_draw,
                       std::span<const entt::entity> entities) {
                gfx::draw draw{ bound_sp, bound_va };

                for (const entt::entity entity : entities) {
                    const auto [maybe_tf, maybe_material] = layer.registry.try_get<game::transform, material>(entity);
                    if (!maybe_tf || !maybe_material) {
                        game::log::warn("entity={} missing transform or material", static_cast<std::uint32_t>(entity));
                        continue;
                    }
                    const auto& tf = *maybe_tf;
                    const auto& a_material = *maybe_material;

                    const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), tf.tr);
                    const glm::mat4 rotation_matrix = glm::mat4_cast(tf.rot);
                    // TODO: scale
                    const glm::mat4 model = translation_matrix * rotation_matrix;
                    const glm::mat3 it_model = glm::transpose(glm::inverse(model));
                    const glm::mat4 transform = camera_frame.projection * camera_frame.view * model;
                    set_model(bound_sp, glm::value_ptr(model));
                    set_it_model(bound_sp, glm::value_ptr(it_model));
                    set_transform(bound_sp, glm::value_ptr(transform));

                    set_material_ambient(bound_sp, a_material.ambient.r, a_material.ambient.g, a_material.ambient.b);
                    set_material_diffuse(bound_sp, a_material.diffuse.r, a_material.diffuse.g, a_material.diffuse.b);
                    set_material_specular(
                        bound_sp, a_material.specular.r, a_material.specular.g, a_material.specular.b
                    );
                    set_material_shininess(bound_sp, a_material.shininess);

                    vertex_draw(draw);
                }
            };
        } },
    };
}

exec::async<std::tuple<entt::entity, ecs::resource<game::shader>&, ecs::resource<game::vertex>&>>
    create_global_entity(game::engine_context& e_ctx, ecs::layer& layer) {
    const entt::entity entity = layer.root;

    layer.registry.emplace<global_entity_state>(entity);

    auto& shader_resource = layer.registry.emplace<ecs::resource<game::shader>::ptr_type>(
        entity, ecs::resource<game::shader>::make(*e_ctx.script_exec)
    );
    auto& vertex_resource = layer.registry.emplace<ecs::resource<game::vertex>::ptr_type>(
        entity, ecs::resource<game::vertex>::make(*e_ctx.script_exec)
    );

    layer.registry.emplace<game::local_transform>(entity);
    layer.registry.emplace<game::transform>(entity);

    layer.registry.emplace<game::input>(
        entity,
        [](ecs::layer& layer, const game::input_events& input_events, entt::entity entity) {
            using action = game::keyboard_input_event::action_type;
            auto& state = *ASSERT_VAL((layer.registry.try_get<global_entity_state>(entity)));
            const sl::meta::pmatch handle{
                [&state](const game::keyboard_input_event& keyboard) {
                    using key = game::keyboard_input_event::key_type;
                    if (keyboard.key == key::ESCAPE) {
                        const bool prev = state.should_close.get().value_or(false);
                        state.should_close.set(prev || keyboard.action == action::PRESS);
                    }
                },
                [](const auto&) {},
            };
            for (const auto& input_event : input_events) {
                handle(input_event);
            }
        }
    );
    layer.registry.emplace<game::update>(entity, [&](ecs::layer& layer, entt::entity entity, game::time_point) {
        auto& state = *ASSERT_VAL((layer.registry.try_get<global_entity_state>(entity)));
        state.should_close.release().map([&cw = e_ctx.w_ctx.current_window](bool should_close) {
            cw.set_should_close(should_close);
        });
    });

    co_return std::tuple<entt::entity, ecs::resource<game::shader>&, ecs::resource<game::vertex>&>{
        entity,
        *shader_resource,
        *vertex_resource,
    };
}

exec::async<entt::entity>
    create_player_entity(game::engine_context& e_ctx, ecs::layer& layer, const game::basis& world) {
    const auto entity = layer.registry.create();

    layer.registry.emplace<player_entity_state>(entity);
    layer.registry.emplace<game::local_transform>(
        entity,
        game::transform{
            .tr{ 0.0f, 0.0f, 3.0f },
            .rot = glm::angleAxis(glm::radians(-180.0f), world.up()),
        }
    );
    layer.registry.emplace<game::camera>(
        entity,
        game::perspective_projection{
            .fov = glm::radians(45.0f),
            .near = 0.1f,
            .far = 100.0f,
        }
    );

    layer.registry.emplace<game::input>(
        entity,
        [](ecs::layer& layer, const game::input_events& input_events, entt::entity entity) {
            using action = game::keyboard_input_event::action_type;
            auto& state = *ASSERT_VAL((layer.registry.try_get<player_entity_state>(entity)));
            const meta::pmatch handle{
                [&state](const game::keyboard_input_event& keyboard) {
                    using key = game::keyboard_input_event::key_type;
                    const bool is_down = keyboard.action == action::PRESS || keyboard.action == action::REPEAT;
                    switch (keyboard.key) {
                    case key::Q:
                        state.q = is_down;
                        break;
                    case key::W:
                        state.w = is_down;
                        break;
                    case key::E:
                        state.e = is_down;
                        break;
                    case key::A:
                        state.a = is_down;
                        break;
                    case key::S:
                        state.s = is_down;
                        break;
                    case key::D:
                        state.d = is_down;
                        break;
                    default:
                        break;
                    }
                },
                [&state](const game::mouse_button_input_event& mouse_button) {
                    using button = game::mouse_button_input_event::button_type;
                    if (mouse_button.button == button::RIGHT) {
                        state.rmb.set_if_ne(
                            mouse_button.action == action::PRESS || mouse_button.action == action::REPEAT
                        );
                    }
                },
                [&state](const game::cursor_input_event& cursor) {
                    if (state.cursor.has_value()) {
                        state.cursor->last = cursor.pos;
                    } else {
                        state.cursor.emplace(cursor_state{
                            .first = cursor.pos,
                            .last = cursor.pos,
                        });
                    }
                },
            };
            for (const auto& input_event : input_events) {
                handle(input_event);
            }
        }
    );
    layer.registry.emplace<game::update>(
        entity,
        [&e_ctx, &world](ecs::layer& layer, entt::entity entity, game::time_point time_point) {
            const auto& [maybe_state, maybe_tf] = layer.registry.try_get<player_entity_state, game::transform>(entity);
            if (!ASSERT_VAL(maybe_state) || !maybe_tf) {
                return;
            }
            player_entity_state& state = *maybe_state;
            game::transform& tf = *maybe_tf;

            const auto& maybe_cursor = std::exchange(state.cursor, meta::null);
            if (maybe_cursor.has_value() && state.rmb.get().value_or(false)) {
                const auto& cursor = maybe_cursor.value();
                const auto cursor_offset = cursor.last - cursor.first;

                constexpr float sensitivity = -glm::radians(0.2f);
                const float yaw = static_cast<float>(cursor_offset.x) * sensitivity;
                const float pitch = static_cast<float>(cursor_offset.y) * sensitivity;

                const glm::quat rot_yaw = glm::angleAxis(yaw, world.y);
                tf.rotate(rot_yaw);

                const glm::vec3 camera_forward = tf.rot * world.forward();
                const glm::vec3 camera_right = glm::cross(camera_forward, world.up());
                const glm::quat rot_pitch = glm::angleAxis(pitch, camera_right);
                tf.rotate(rot_pitch);
            }

            {
                constexpr float acc = 5.0f;
                const float speed = acc * time_point.delta_sec().count();
                const auto new_tr = [&state, &world] {
                    constexpr auto sub = [](bool a, bool b) {
                        return static_cast<float>(static_cast<int>(a) - static_cast<int>(b));
                    };
                    return sub(state.e, state.q) * world.up() + //
                           sub(state.d, state.a) * world.right() + //
                           sub(state.w, state.s) * world.forward();
                }();
                tf.translate(speed * (tf.rot * new_tr));
            }

            state.rmb.get().map([&cw = e_ctx.w_ctx.current_window](bool rmb) {
                cw.set_input_mode(GLFW_CURSOR, rmb ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            });
        }
    );

    co_return entity;
}

exec::async<entt::entity>
    create_source_entity(script::example_context& example_ctx, ecs::layer& layer, const game::basis& world) {
    const auto entity = layer.registry.create();

    layer.registry.emplace<game::shader::id>(entity, "source.shader"_us(example_ctx.uss));
    layer.registry.emplace<game::vertex::id>(entity, "cube.vertex"_us(example_ctx.uss));
    layer.registry.emplace<source_entity_state>(
        entity,
        source_entity_state{
            .ambient{ 0.2f, 0.2f, 0.2f },
            .diffuse{ 0.5f, 0.5f, 0.5f },
            .specular{ 1.0f, 1.0f, 1.0f },
        }
    );
    layer.registry.emplace<game::local_transform>(
        entity,
        game::transform{
            .tr{ 2.0f, 2.0f, -3.0f },
            .rot = glm::angleAxis(0.0f, world.up()),
        }
    );
    layer.registry.emplace<game::overlay>(entity, [](ecs::layer& layer, gfx::imgui_frame&, entt::entity entity) {
        auto [maybe_state, maybe_local_transform] =
            layer.registry.try_get<source_entity_state, game::local_transform>(entity);
        if (!maybe_state || !maybe_local_transform) {
            return;
        }
        auto& state = *maybe_state;
        auto& local_transform = *maybe_local_transform;

        if (local_transform.get().has_value()) {
            game::transform tf = local_transform.get().value();
            if (ImGui::SliderFloat3("light position", glm::value_ptr(tf.tr), -10.0f, 10.0f)) {
                local_transform.set(tf);
            }
        }
        ImGui::ColorEdit3("light ambient", glm::value_ptr(state.ambient));
        ImGui::ColorEdit3("light diffuse", glm::value_ptr(state.diffuse));
        ImGui::ColorEdit3("light specular", glm::value_ptr(state.specular));
    });

    co_return entity;
}

constexpr std::array cube_vertices{
    // verts            | normals
    // front face
    VN{ { +0.5f, +0.5f, +0.5f }, { +0.0f, +0.0f, +1.0f } }, // top right
    VN{ { +0.5f, -0.5f, +0.5f }, { +0.0f, +0.0f, +1.0f } }, // bottom right
    VN{ { -0.5f, -0.5f, +0.5f }, { +0.0f, +0.0f, +1.0f } }, // bottom left
    VN{ { -0.5f, +0.5f, +0.5f }, { +0.0f, +0.0f, +1.0f } }, // top left
    // right face
    VN{ { +0.5f, +0.5f, +0.5f }, { +1.0f, +0.0f, +0.0f } }, // top right
    VN{ { +0.5f, -0.5f, +0.5f }, { +1.0f, +0.0f, +0.0f } }, // bottom right
    VN{ { +0.5f, -0.5f, -0.5f }, { +1.0f, +0.0f, +0.0f } }, // bottom left
    VN{ { +0.5f, +0.5f, -0.5f }, { +1.0f, +0.0f, +0.0f } }, // top left
    // back face
    VN{ { +0.5f, +0.5f, -0.5f }, { +0.0f, +0.0f, -1.0f } }, // top right
    VN{ { +0.5f, -0.5f, -0.5f }, { +0.0f, +0.0f, -1.0f } }, // bottom right
    VN{ { -0.5f, -0.5f, -0.5f }, { +0.0f, +0.0f, -1.0f } }, // bottom left
    VN{ { -0.5f, +0.5f, -0.5f }, { +0.0f, +0.0f, -1.0f } }, // top left
    // left face
    VN{ { -0.5f, +0.5f, -0.5f }, { -1.0f, +0.0f, +0.0f } }, // top right
    VN{ { -0.5f, -0.5f, -0.5f }, { -1.0f, +0.0f, +0.0f } }, // bottom right
    VN{ { -0.5f, -0.5f, +0.5f }, { -1.0f, +0.0f, +0.0f } }, // bottom left
    VN{ { -0.5f, +0.5f, +0.5f }, { -1.0f, +0.0f, +0.0f } }, // top left
    // top face
    VN{ { +0.5f, +0.5f, +0.5f }, { +0.0f, +1.0f, +0.0f } }, // top right
    VN{ { +0.5f, +0.5f, -0.5f }, { +0.0f, +1.0f, +0.0f } }, // bottom right
    VN{ { -0.5f, +0.5f, -0.5f }, { +0.0f, +1.0f, +0.0f } }, // bottom left
    VN{ { -0.5f, +0.5f, +0.5f }, { +0.0f, +1.0f, +0.0f } }, // top left
    // bottom face
    VN{ { +0.5f, -0.5f, +0.5f }, { +0.0f, -1.0f, +0.0f } }, // top right
    VN{ { +0.5f, -0.5f, -0.5f }, { +0.0f, -1.0f, +0.0f } }, // bottom right
    VN{ { -0.5f, -0.5f, -0.5f }, { +0.0f, -1.0f, +0.0f } }, // bottom left
    VN{ { -0.5f, -0.5f, +0.5f }, { +0.0f, -1.0f, +0.0f } }, // top left
};

constexpr std::array cube_indices{
    0u,  1u,  3u,  1u,  2u,  3u, // Front face
    4u,  5u,  7u,  5u,  6u,  7u, // Right face
    8u,  9u,  11u, 9u,  10u, 11u, // Back face
    12u, 13u, 15u, 13u, 14u, 15u, // Left face
    16u, 17u, 19u, 17u, 18u, 19u, // Top face
    20u, 21u, 23u, 21u, 22u, 23u, // Bottom face
};

struct PM {
    glm::vec3 pos;
    material mat;
};

constexpr std::array cube_objects{
    PM{
        .pos{ 0.0f, 0.0f, 0.0f },
        .mat{
            .ambient{ 0.1745f, 0.01175f, 0.01175f },
            .diffuse{ 0.61424f, 0.04136f, 0.04136f },
            .specular{ 0.727811f, 0.626959f, 0.626959f },
            .shininess = 128.0f * 0.6f,
        },
    },
    PM{
        .pos{ 5.0f, 0.0f, 0.0f },
        .mat{
            .ambient{ 1.0f, 0.5f, 0.31f },
            .diffuse{ 1.0f, 0.5f, 0.31f },
            .specular{ 0.5f, 0.5f, 0.5f },
            .shininess = 32.0f,
        },
    },
};

exec::async<std::vector<entt::entity>>
    create_cube_entities(script::example_context& example_ctx, ecs::layer& layer, const game::basis& world) {
    std::vector<entt::entity> entities;

    const auto object_shader_id = "object.shader"_us(example_ctx.uss);
    const auto cube_vertex_id = "cube.vertex"_us(example_ctx.uss);

    for (const auto& [pos, mat] : cube_objects) {
        entt::entity entity = layer.registry.create();

        layer.registry.emplace<game::shader::id>(entity, object_shader_id);
        layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
        layer.registry.emplace<game::local_transform>(
            entity,
            game::transform{
                .tr = pos,
                .rot = glm::angleAxis(0.0f, world.up()),
            }
        );
        layer.registry.emplace<material>(entity, mat);
        layer.registry.emplace<game::update>(
            entity,
            [&world](ecs::layer& layer, entt::entity entity, game::time_point time_point) {
                const float t = time_point.now_sec().count();
                const float angle_v = 2.f;
                const float angle = angle_v * t;
                auto& local_tf = layer.registry.get<game::local_transform>(entity);
                local_tf.set(game::transform{
                    .tr = local_tf.get()->tr,
                    .rot = glm::angleAxis(glm::radians(angle), world.up()),
                });
            }
        );
        layer.registry.emplace<game::overlay>(entity, [](ecs::layer& layer, gfx::imgui_frame&, entt::entity entity) {
            auto* maybe_material = layer.registry.try_get<material>(entity);
            if (!ASSERT_VAL(maybe_material)) {
                return;
            }
            auto& a_material = *maybe_material;

            ImGui::PushID(static_cast<int>(entity));
            ImGui::Spacing();
            ImGui::ColorEdit3("ambient", glm::value_ptr(a_material.ambient));
            ImGui::ColorEdit3("diffuse", glm::value_ptr(a_material.diffuse));
            ImGui::ColorEdit3("specular", glm::value_ptr(a_material.specular));
            ImGui::SliderFloat("shininess", &a_material.shininess, 2.0f, 256, "%.0f", ImGuiSliderFlags_Logarithmic);
            ImGui::PopID();
        });

        entities.push_back(entity);
    }

    co_return entities;
}

exec::async<void> create_scene(
    game::engine_context& e_ctx,
    script::example_context& example_ctx,
    ecs::layer& layer,
    const game::basis& world
) {
    auto [global_entity, shader_resource, vertex_resource] = co_await create_global_entity(e_ctx, layer);
    // common vvv
    std::ignore =
        co_await shader_resource.require("object.shader"_us(example_ctx.uss), create_object_shader(example_ctx));
    std::ignore =
        co_await shader_resource.require("source.shader"_us(example_ctx.uss), create_source_shader(example_ctx));
    std::ignore = co_await vertex_resource.require(
        "cube.vertex"_us(example_ctx.uss), script::create_vertex(std::span(cube_vertices), std::span(cube_indices))
    );
    // common ^^^

    const auto player_entity = co_await create_player_entity(e_ctx, layer, world);
    game::node::attach_child(layer, global_entity, player_entity);

    const auto source_entity = co_await create_source_entity(example_ctx, layer, world);
    game::node::attach_child(layer, global_entity, source_entity);

    const auto cube_entities = co_await create_cube_entities(example_ctx, layer, world);
    game::node::attach_children(layer, global_entity, std::span{ cube_entities });
}

void main(int argc, char** argv) {
    spdlog::set_level(spdlog::level::info);

    auto w_ctx = *ASSERT_VAL(
        game::window_context::initialize(sl::meta::null, "02_material", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f })
    );
    auto e_ctx = game::engine_context::initialize(std::move(w_ctx), argc, argv);
    auto example_ctx = script::example_context::create(e_ctx);
    ecs::layer layer{};
    game::graphics_system gfx_system{ .layer = layer, .world{} };
    game::overlay_system overlay_system{ .layer = layer };

    exec::coro_schedule(*e_ctx.script_exec, create_scene(e_ctx, example_ctx, layer, gfx_system.world));

    while (!e_ctx.w_ctx.current_window.should_close()) {
        // input
        e_ctx.w_ctx.context->poll_events();
        e_ctx.w_ctx.state->frame_buffer_size.release().map( //
            [&cw = e_ctx.w_ctx.current_window](const glm::ivec2& frame_buffer_size) {
                cw.viewport(glm::ivec2{}, frame_buffer_size);
            }
        );
        e_ctx.w_ctx.state->window_content_scale
            .release() //
            .map([&overlay_system](const glm::fvec2& window_content_scale) {
                overlay_system.on_window_content_scale_changed(window_content_scale);
            });
        e_ctx.in_sys->process(layer);

        const game::time_point& time_point = e_ctx.time_calculate();

        // update and execute scripts
        std::ignore = e_ctx.script_exec->execute_batch();
        game::update_system(layer, time_point);

        // transform update
        game::local_transform_system(layer, time_point);

        // render
        const auto window_frame = e_ctx.w_ctx.new_frame();
        gfx_system.execute(window_frame);

        // overlay
        auto imgui_frame = e_ctx.w_ctx.imgui.new_frame();
        if (auto imgui_window = imgui_frame.begin("entities")) {
            overlay_system.execute(imgui_frame);
        }
    }
}

} // namespace sl

int main(int argc, char** argv) {
    sl::main(argc, argv);
    return 0;
}
