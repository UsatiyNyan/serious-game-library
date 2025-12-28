//
// Created by usatiynyan.
//

#include "common.hpp"
#include <fastgltf/core.hpp>
#include <sl/exec/algo/make/result.hpp>

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

exec::async<game::shader> create_unlit_shader(const script::example_context& example_ctx) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, example_ctx.asset_path / "shaders/unlit.vert")
        ),
        *ASSERT_VAL(
            gfx::shader::load_from_file(gfx::shader_type::fragment, example_ctx.asset_path / "shaders/unlit.frag")
        ),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();

    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));
    auto set_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_color"));
    co_return game::shader{
        .sp{ std::move(sp) },
        .setup{ [ //
                    set_transform = std::move(set_transform),
                    set_color = std::move(set_color)](
                    const ecs::layer& layer, //
                    const game::camera_frame& camera_frame,
                    const gfx::bound_shader_program& bound_sp
                ) mutable {
            return [&](const gfx::bound_vertex_array& bound_va,
                       game::vertex::draw_type& vertex_draw,
                       std::span<const entt::entity> entities) {
                gfx::draw draw{ bound_sp, bound_va };

                for (const entt::entity entity : entities) {
                    const auto [maybe_tf, maybe_pl] =
                        layer.registry.try_get<game::transform, game::render::point_light>(entity);
                    if (!maybe_tf || !maybe_pl) {
                        continue;
                    }
                    const auto& tf = *maybe_tf;
                    const auto& pl = *maybe_pl;

                    const glm::mat4 transform =
                        camera_frame.projection * camera_frame.view * glm::translate(glm::mat4(1.0f), tf.tr);
                    set_transform(bound_sp, transform);
                    set_color(bound_sp, pl.ambient);

                    vertex_draw(draw);
                }
            };
        } },
    };
}

exec::async<game::shader> create_object_shader(const script::example_context& example_ctx, const game::basis& world) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(
            gfx::shader::load_from_file(gfx::shader_type::vertex, example_ctx.asset_path / "shaders/blinn_phong.vert")
        ),
        *ASSERT_VAL(
            gfx::shader::load_from_file(gfx::shader_type::fragment, example_ctx.asset_path / "shaders/blinn_phong.frag")
        ),
    };

    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();

    constexpr std::array<std::string_view, 2> material_textures{
        "u_material.diffuse",
        "u_material.specular",
    };
    sp_bind.initialize_tex_units(std::span{ material_textures });

    auto set_view_pos = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_view_pos"));

    auto dl_buffer = game::make_and_initialize_ssbo<game::render::directional_light_element>(1);
    auto pl_buffer = game::make_and_initialize_ssbo<game::render::point_light_element>(16);
    auto sl_buffer = game::make_and_initialize_ssbo<game::render::spot_light_element>(1);
    auto set_dl_size = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1ui, "u_directional_light_size"));
    auto set_pl_size = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1ui, "u_point_light_size"));
    auto set_sl_size = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1ui, "u_spot_light_size"));

    auto set_material_diffuse_color =
        *ASSERT_VAL(sp_bind.make_uniform_v_setter(glUniform4fv, "u_material.diffuse_color", 1));
    auto set_material_specular_color =
        *ASSERT_VAL(sp_bind.make_uniform_v_setter(glUniform4fv, "u_material.specular_color", 1));
    auto set_material_mode = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1ui, "u_material.mode"));
    auto set_material_shininess = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1f, "u_material.shininess"));

    auto set_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_model", 1, false));
    auto set_it_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix3fv, "u_it_model", 1, false));
    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));

    co_return game::shader{
        .sp{ std::move(sp) },
        .setup{ [ //
                    world,
                    set_view_pos = std::move(set_view_pos),

                    dl_buffer = std::move(dl_buffer),
                    pl_buffer = std::move(pl_buffer),
                    sl_buffer = std::move(sl_buffer),
                    set_dl_size = std::move(set_dl_size),
                    set_pl_size = std::move(set_pl_size),
                    set_sl_size = std::move(set_sl_size),

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
            set_view_pos(bound_sp, camera_frame.position);

            const std::uint32_t dl_size = game::fill_ssbo(layer, world, dl_buffer);
            set_dl_size(bound_sp, dl_size);

            const std::uint32_t pl_size = game::fill_ssbo(layer, world, pl_buffer);
            set_pl_size(bound_sp, pl_size);

            const std::uint32_t sl_size = game::fill_ssbo(layer, world, sl_buffer);
            set_sl_size(bound_sp, sl_size);

            auto* const maybe_mat_resource =
                layer.registry.try_get<ecs::resource<game::material>::ptr_type>(layer.root);

            return [&,
                    maybe_mat_resource,
                    bound_dl_base = dl_buffer.bind_base(0),
                    bound_pl_base = pl_buffer.bind_base(1),
                    bound_sl_base = sl_buffer.bind_base(2)]( //
                       const gfx::bound_vertex_array& bound_va,
                       game::vertex::draw_type& vertex_draw,
                       std::span<const entt::entity> entities
                   ) {
                for (const entt::entity entity : entities) {
                    const auto [maybe_tf, maybe_mat_id] =
                        layer.registry.try_get<game::transform, game::material::id>(entity);
                    if (!maybe_tf || !maybe_mat_id) {
                        game::log::warn("entity={} missing transform or material", static_cast<std::uint32_t>(entity));
                        continue;
                    }
                    const auto& tf = *maybe_tf;
                    const auto& mat_id = *maybe_mat_id;

                    const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), tf.tr);
                    const glm::mat4 rotation_matrix = glm::mat4_cast(tf.rot);
                    // TODO: scale
                    const glm::mat4 model = translation_matrix * rotation_matrix;
                    const glm::mat3 it_model = glm::transpose(glm::inverse(model));
                    const glm::mat4 transform = camera_frame.projection * camera_frame.view * model;
                    set_model(bound_sp, model);
                    set_it_model(bound_sp, it_model);
                    set_transform(bound_sp, transform);

                    if (maybe_mat_resource == nullptr) [[unlikely]] {
                        continue;
                    }
                    auto& mat_resource = **maybe_mat_resource;

                    const auto maybe_mat = mat_resource.lookup_unsafe(mat_id.id);
                    if (!maybe_mat.has_value()) [[unlikely]] {
                        continue;
                    }
                    const auto& mat = maybe_mat.value();

                    const auto maybe_bound_diffuse_tex =
                        mat->diffuse
                        | meta::pmatch{
                              [](const meta::persistent<game::texture>& tex) -> meta::maybe<gfx::bound_texture> {
                                  return tex->tex.activate(0);
                              },
                              [&](const glm::vec4& clr) -> meta::maybe<gfx::bound_texture> {
                                  set_material_diffuse_color(bound_sp, clr);
                                  return meta::null;
                              },
                          };
                    const auto maybe_bound_specular_tex =
                        mat->specular
                        | meta::pmatch{
                              [](const meta::persistent<game::texture>& tex) -> meta::maybe<gfx::bound_texture> {
                                  return tex->tex.activate(1);
                              },
                              [&](const glm::vec4& clr) -> meta::maybe<gfx::bound_texture> {
                                  set_material_specular_color(bound_sp, clr);
                                  return meta::null;
                              },
                          };

                    const std::uint32_t mat_mode = (maybe_bound_diffuse_tex.has_value() ? 0b01 : 0)
                                                   + (maybe_bound_specular_tex.has_value() ? 0b10 : 0);
                    set_material_mode(bound_sp, mat_mode);
                    set_material_shininess(bound_sp, mat->shininess);

                    gfx::draw draw{ bound_sp, bound_va };
                    vertex_draw(draw);
                }
            };
        } },
    };
}

exec::async<entt::entity>
    create_global_entity(game::engine_context& e_ctx, script::example_context& example_ctx, ecs::layer& layer) {
    const entt::entity entity = layer.root;

    layer.registry.emplace<global_entity_state>(entity);

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
        auto& state = layer.registry.get<global_entity_state>(entity);
        state.should_close.release().map([&cw = e_ctx.w_ctx.current_window](bool should_close) {
            cw.set_should_close(should_close);
        });
    });

    const auto crate_material_id = "material.crate"_us(example_ctx.uss);
    layer.registry.emplace<game::overlay>(
        entity,
        [crate_material_id](ecs::layer& layer, gfx::imgui_frame&, entt::entity entity) {
            auto* const maybe_mat_resource = layer.registry.try_get<ecs::resource<game::material>::ptr_type>(entity);
            if (maybe_mat_resource != nullptr) {
                auto& mat_resource = **maybe_mat_resource;
                auto maybe_mat = mat_resource.lookup_unsafe(crate_material_id);
                if (maybe_mat.has_value()) {
                    auto& mat = maybe_mat.value();

                    ImGui::Spacing();
                    ImGui::SliderFloat("shininess", &mat->shininess, 2.0f, 256, "%.0f", ImGuiSliderFlags_Logarithmic);
                }
            }
        }
    );

    co_return entity;
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
    layer.registry.emplace<game::render::spot_light>(
        entity,
        game::render::spot_light{
            .ambient{ 0.0f, 0.0f, 0.0f },
            .diffuse{ 1.0f, 1.0f, 1.0f },
            .specular{ 1.0f, 1.0f, 1.0f },

            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f,

            .cutoff = glm::cos(glm::radians(12.5f)),
            .outer_cutoff = glm::cos(glm::radians(15.0f)),
        }
    );

    co_return entity;
}

constexpr std::array point_light_positions{
    glm::vec3{ 10.0f, 0.0f, 0.0f },
    glm::vec3{ 0.0f, 10.0f, 0.0f },
    glm::vec3{ 0.0f, 0.0f, 10.0f },
};

constexpr std::array point_lights{
    game::render::point_light{
        .ambient{ 1.0f, 0.0f, 0.0f },
        .diffuse{ 1.0f, 0.0f, 0.0f },
        .specular{ 1.0f, 0.0f, 0.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
    game::render::point_light{
        .ambient{ 0.0f, 1.0f, 0.0f },
        .diffuse{ 0.0f, 1.0f, 0.0f },
        .specular{ 0.0f, 1.0f, 0.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
    game::render::point_light{
        .ambient{ 0.0f, 0.0f, 1.0f },
        .diffuse{ 0.0f, 0.0f, 1.0f },
        .specular{ 0.0f, 0.0f, 1.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
};

exec::async<std::vector<entt::entity>>
    create_light_entities(game::engine_context& e_ctx, script::example_context& example_ctx, ecs::layer& layer) {
    const auto cube_vertex_id = "vertex.cube"_us(example_ctx.uss);
    const auto unlit_shader_id = "shader.unlit"_us(example_ctx.uss);

    std::vector<entt::entity> entities;

    for (const auto& [p, l] : ranges::views::zip(point_light_positions, point_lights)) {
        entt::entity entity = layer.registry.create();
        layer.registry.emplace<game::shader::id>(entity, unlit_shader_id);
        layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
        layer.registry.emplace<game::local_transform>(entity, game::transform{ .tr = p });
        layer.registry.emplace<game::render::point_light>(entity, l);
        entities.push_back(entity);
    }
    // example of "alternative" update
    exec::coro_schedule(
        *e_ctx.script_exec,
        [](game::engine_context& e_ctx, ecs::layer& layer, std::vector<entt::entity> entities) -> exec::async<void> {
            while (e_ctx.is_ok()) {
                const auto maybe_tp = co_await e_ctx.next_frame();
                if (!maybe_tp.has_value()) {
                    continue;
                }
                const auto& tp = maybe_tp.value();

                const float coef = std::sin(tp.delta_from_init_sec().count());
                for (const auto& [initial, pl_entity] : ranges::views::zip(point_lights, entities)) {
                    auto& current = layer.registry.get<game::render::point_light>(pl_entity);
                    current.ambient = initial.ambient * coef;
                    current.diffuse = initial.diffuse * coef;
                    current.specular = initial.specular * coef;
                }
            }
        }(e_ctx, layer, entities)
    );

    {
        const entt::entity entity = layer.registry.create();

        layer.registry.emplace<game::local_transform>(entity);
        layer.registry.emplace<game::render::directional_light>(
            entity,
            game::render::directional_light{
                .ambient{ 0.05f, 0.05f, 0.05f },
                .diffuse{ 0.4f, 0.4f, 0.4f },
                .specular{ 0.5f, 0.5f, 0.5f },
            }
        );
        layer.registry.emplace<game::overlay>(entity, [](ecs::layer& layer, gfx::imgui_frame&, entt::entity entity) {
            auto& local_transform = layer.registry.get<game::local_transform>(entity);

            if (const auto& maybe_transform = local_transform.get()) {
                auto eulerAngles = glm::eulerAngles(maybe_transform->rot);
                constexpr auto pi = std::numbers::pi_v<float>;
                if (ImGui::SliderFloat3("directional_light eulerAngle", glm::value_ptr(eulerAngles), -pi, pi)) {
                    auto transform = maybe_transform.value();
                    transform.rot = glm::quat{ eulerAngles };
                    local_transform.set(std::move(transform));
                }
            }

            auto& directional_light = layer.registry.get<game::render::directional_light>(entity);
            ImGui::ColorEdit3("directional_light ambient", glm::value_ptr(directional_light.ambient));
            ImGui::ColorEdit3("directional_light diffuse", glm::value_ptr(directional_light.diffuse));
            ImGui::ColorEdit3("directional_light specular", glm::value_ptr(directional_light.specular));
        });

        entities.push_back(entity);
    }

    co_return entities;
}

constexpr std::array cube_vertices{
    // verts            | normals
    // front face
    VNT{ { +0.5f, +0.5f, +0.5f }, { +0.0f, +0.0f, +1.0f }, { +1.0f, +1.0f } }, // top right
    VNT{ { +0.5f, -0.5f, +0.5f }, { +0.0f, +0.0f, +1.0f }, { +1.0f, +0.0f } }, // bottom right
    VNT{ { -0.5f, -0.5f, +0.5f }, { +0.0f, +0.0f, +1.0f }, { +0.0f, +0.0f } }, // bottom left
    VNT{ { -0.5f, +0.5f, +0.5f }, { +0.0f, +0.0f, +1.0f }, { +0.0f, +1.0f } }, // top left
    // right face
    VNT{ { +0.5f, +0.5f, +0.5f }, { +1.0f, +0.0f, +0.0f }, { +1.0f, +1.0f } }, // top right
    VNT{ { +0.5f, -0.5f, +0.5f }, { +1.0f, +0.0f, +0.0f }, { +1.0f, +0.0f } }, // bottom right
    VNT{ { +0.5f, -0.5f, -0.5f }, { +1.0f, +0.0f, +0.0f }, { +0.0f, +0.0f } }, // bottom left
    VNT{ { +0.5f, +0.5f, -0.5f }, { +1.0f, +0.0f, +0.0f }, { +0.0f, +1.0f } }, // top left
    // back face
    VNT{ { +0.5f, +0.5f, -0.5f }, { +0.0f, +0.0f, -1.0f }, { +1.0f, +1.0f } }, // top right
    VNT{ { +0.5f, -0.5f, -0.5f }, { +0.0f, +0.0f, -1.0f }, { +1.0f, +0.0f } }, // bottom right
    VNT{ { -0.5f, -0.5f, -0.5f }, { +0.0f, +0.0f, -1.0f }, { +0.0f, +0.0f } }, // bottom left
    VNT{ { -0.5f, +0.5f, -0.5f }, { +0.0f, +0.0f, -1.0f }, { +0.0f, +1.0f } }, // top left
    // left face
    VNT{ { -0.5f, +0.5f, -0.5f }, { -1.0f, +0.0f, +0.0f }, { +1.0f, +1.0f } }, // top right
    VNT{ { -0.5f, -0.5f, -0.5f }, { -1.0f, +0.0f, +0.0f }, { +1.0f, +0.0f } }, // bottom right
    VNT{ { -0.5f, -0.5f, +0.5f }, { -1.0f, +0.0f, +0.0f }, { +0.0f, +0.0f } }, // bottom left
    VNT{ { -0.5f, +0.5f, +0.5f }, { -1.0f, +0.0f, +0.0f }, { +0.0f, +1.0f } }, // top left
    // top face
    VNT{ { +0.5f, +0.5f, +0.5f }, { +0.0f, +1.0f, +0.0f }, { +1.0f, +1.0f } }, // top right
    VNT{ { +0.5f, +0.5f, -0.5f }, { +0.0f, +1.0f, +0.0f }, { +1.0f, +0.0f } }, // bottom right
    VNT{ { -0.5f, +0.5f, -0.5f }, { +0.0f, +1.0f, +0.0f }, { +0.0f, +0.0f } }, // bottom left
    VNT{ { -0.5f, +0.5f, +0.5f }, { +0.0f, +1.0f, +0.0f }, { +0.0f, +1.0f } }, // top left
    // bottom face
    VNT{ { +0.5f, -0.5f, +0.5f }, { +0.0f, -1.0f, +0.0f }, { +1.0f, +1.0f } }, // top right
    VNT{ { +0.5f, -0.5f, -0.5f }, { +0.0f, -1.0f, +0.0f }, { +1.0f, +0.0f } }, // bottom right
    VNT{ { -0.5f, -0.5f, -0.5f }, { +0.0f, -1.0f, +0.0f }, { +0.0f, +0.0f } }, // bottom left
    VNT{ { -0.5f, -0.5f, +0.5f }, { +0.0f, -1.0f, +0.0f }, { +0.0f, +1.0f } }, // top left
};

constexpr std::array cube_indices{
    0u,  1u,  3u,  1u,  2u,  3u, // Front face
    4u,  5u,  7u,  5u,  6u,  7u, // Right face
    8u,  9u,  11u, 9u,  10u, 11u, // Back face
    12u, 13u, 15u, 13u, 14u, 15u, // Left face
    16u, 17u, 19u, 17u, 18u, 19u, // Top face
    20u, 21u, 23u, 21u, 22u, 23u, // Bottom face
};

exec::async<std::vector<entt::entity>>
    create_cube_entities(script::example_context& example_ctx, ecs::layer& layer, const game::basis& world) {
    const auto object_shader_id = "shader.object"_us(example_ctx.uss);
    const auto cube_vertex_id = "vertex.cube"_us(example_ctx.uss);
    const auto crate_material_id = "material.crate"_us(example_ctx.uss);
    const auto solid_material_id = "material.solid"_us(example_ctx.uss);

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
    entities.reserve(rows * cols + 1);

    auto generator = generate_positions(rows, cols);
    for (const auto [index, position] : ranges::views::enumerate(generator)) {
        entt::entity entity = layer.registry.create();

        layer.registry.emplace<game::shader::id>(entity, object_shader_id);
        layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
        layer.registry.emplace<game::material::id>(entity, crate_material_id);

        const float angle0 = 20.0f * (static_cast<float>(index));
        layer.registry.emplace<game::local_transform>(
            entity,
            game::transform{
                .tr = position,
                .rot = glm::angleAxis(0.0f, world.up()),
            }
        );
        layer.registry.emplace<game::update>(
            entity,
            [&world, angle0](ecs::layer& layer, entt::entity entity, game::time_point tp) {
                const float t = tp.now_sec().count();
                const float angle_v = 2.f;
                const float angle = angle0 + (angle_v * t);
                auto& local_tf = layer.registry.get<game::local_transform>(entity);
                local_tf.set(game::transform{
                    .tr = local_tf.get()->tr,
                    .rot = glm::angleAxis(glm::radians(angle), world.up()),
                });
            }
        );

        entities.push_back(entity);
    }

    {
        entt::entity entity = layer.registry.create();

        layer.registry.emplace<game::shader::id>(entity, object_shader_id);
        layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
        layer.registry.emplace<game::material::id>(entity, solid_material_id);
        layer.registry.emplace<game::local_transform>(entity, game::transform{ .tr{ 0.0f, 2.0f, 0.0f } });

        entities.push_back(entity);
    }


    co_return entities;
}

exec::async<entt::entity> create_imported_entity(
    script::example_context& example_ctx,
    ecs::layer& layer,
    const std::filesystem::path& asset_relpath
) {
    const auto object_shader_id = "shader.object"_us(example_ctx.uss);
    const auto crate_material_id = "material.crate"_us(example_ctx.uss);
    const auto default_material_id = crate_material_id;

    auto& vertex_resource = *layer.registry.get<ecs::resource<game::vertex>::ptr_type>(layer.root);
    auto& texture_resource = *layer.registry.get<ecs::resource<game::texture>::ptr_type>(layer.root);
    auto& material_resource = *layer.registry.get<ecs::resource<game::material>::ptr_type>(layer.root);
    auto& mesh_resource = *layer.registry.get<ecs::resource<game::mesh>::ptr_type>(layer.root);
    auto& primitive_resource = *layer.registry.get<ecs::resource<game::primitive>::ptr_type>(layer.root);

    using namespace fastgltf;
    thread_local Parser parser;

    const auto asset_path = example_ctx.asset_path / asset_relpath;
    const auto asset_id = asset_relpath.generic_string();
    const auto asset_directory = asset_path.parent_path();
    const auto asset = [&] {
        auto data_buffer = *ASSERT_VAL(GltfDataBuffer::FromPath(asset_path));
        auto asset = *ASSERT_VAL(parser.loadGltf(data_buffer, asset_directory));
        return asset;
    }();

    for (const auto& [material_i, material] : ranges::views::enumerate(asset.materials)) {
        const auto& material_pbr = material.pbrData;
        const auto& material_texture = material_pbr.baseColorTexture;

        const auto material_id = "{}.material[{}]"_ufs(asset_id, material_i)(example_ctx.uss);
        auto diffuse = co_await [&]() -> exec::async<game::texture_or_color> {
            if (!material_texture.has_value()) {
                const auto color = std::bit_cast<glm::vec4>(material_pbr.baseColorFactor);
                game::log::debug("color={}", glm::to_string(color));
                co_return color;
            }

            const auto& texture = asset.textures.at(material_texture->textureIndex);
            const auto& image = asset.images.at(*ASSERT_VAL(texture.imageIndex));
            const auto& image_uri = *ASSERT_VAL(std::get_if<sources::URI>(&image.data));
            const auto texture_path = asset_directory / image_uri.uri.fspath();
            const auto texture_id = "{}.texture"_ufs(image_uri.uri.string())(example_ctx.uss);
            game::log::debug("texture_id={}", texture_id);
            co_return *ASSERT_VAL(
                co_await texture_resource.require(texture_id, script::create_texture(texture_path, false))
            );
        }();

        ASSERT(co_await material_resource.require(
            material_id,
            exec::value_as_signal(game::material{
                .diffuse = std::move(diffuse),
                .specular = glm::vec4{ 0.05f },
                .shininess = 128.0f * 0.6f,
            })
        ));
        game::log::debug("material_id={}", material_id);
    }

    const auto load_bin = [asset_directory](const std::filesystem::path& fspath) {
        using iterator = std::istream_iterator<char>;

        std::ifstream bin_file{ asset_directory / fspath, std::ios::binary };
        std::vector<char> output{ iterator{ bin_file }, iterator{} };
        ASSERT(output.size() > 0);
        return output;
    };

    for (const auto& [mesh_i, mesh_] : ranges::views::enumerate(asset.meshes)) {
        const auto mesh_id = "{}.mesh[{}]"_ufs(asset_id, mesh_i)(example_ctx.uss);
        std::ignore = co_await mesh_resource.require(mesh_id, [&, &mesh = mesh_]() -> exec::async<game::mesh> {
            game::mesh mesh_asset;

            for (const auto& [primitive_i, primitive] : ranges::views::enumerate(mesh.primitives)) {
                const auto primitive_id = "{}.primitive[{}]"_ufs(mesh_id, primitive_i)(example_ctx.uss);
                const auto primitive_vertex_id = "{}.vertex"_ufs(primitive_id)(example_ctx.uss);
                const auto primitive_material_id = [&, primitive_material_i = primitive.materialIndex] {
                    if (!primitive_material_i.has_value()) {
                        return default_material_id;
                    }
                    // TODO: FIX THIS
                    auto material_id = "{}.material[{}]"_ufs(asset_id, primitive_material_i.value())(example_ctx.uss);
                    if (!material_resource.lookup_unsafe(material_id).has_value()) {
                        return default_material_id;
                    }
                    return material_id;
                }();

                const auto vert_attr = primitive.findAttribute("POSITION")->accessorIndex;
                const auto& vert_accessor = asset.accessors.at(vert_attr);
                ASSERT(vert_accessor.componentType == ComponentType::Float);
                ASSERT(vert_accessor.type == AccessorType::Vec3);
                ASSERT(vert_accessor.count > 0);
                const auto& vert_buffer_view = asset.bufferViews.at(*ASSERT_VAL(vert_accessor.bufferViewIndex));
                const auto& vert_buffer = asset.buffers.at(vert_buffer_view.bufferIndex);
                const auto& vert_buffer_uri = *ASSERT_VAL(std::get_if<sources::URI>(&vert_buffer.data));
                const auto vert_buffer_data = load_bin(vert_buffer_uri.uri.fspath());
                const auto vert_buffer_data_bytes = std::span{ vert_buffer_data }.subspan(
                    vert_buffer_view.byteOffset + vert_accessor.byteOffset, vert_buffer_view.byteLength
                );
                const auto vert_at = [vert_buffer_data_bytes,
                                      byte_stride = vert_buffer_view.byteStride,
                                      count = vert_accessor.count](std::size_t i) {
                    using element_type = gfx::va_attrib_field<3, float>;
                    ASSERT(i < count);
                    const auto address = byte_stride.value_or(sizeof(element_type)) * i;
                    ASSERT(address < vert_buffer_data_bytes.size_bytes());
                    return *std::bit_cast<const element_type*>(vert_buffer_data_bytes.data() + address);
                };

                const auto normal_attr = primitive.findAttribute("NORMAL")->accessorIndex;
                const auto& normal_accessor = asset.accessors.at(normal_attr);
                ASSERT(normal_accessor.componentType == ComponentType::Float);
                ASSERT(normal_accessor.type == AccessorType::Vec3);
                ASSERT(normal_accessor.count > 0);
                const auto& normal_buffer_view = asset.bufferViews.at(*ASSERT_VAL(normal_accessor.bufferViewIndex));
                const auto& normal_buffer = asset.buffers.at(normal_buffer_view.bufferIndex);
                const auto& normal_buffer_uri = *ASSERT_VAL(std::get_if<sources::URI>(&normal_buffer.data));
                const auto normal_buffer_data = load_bin(normal_buffer_uri.uri.fspath());
                const auto normal_buffer_data_bytes = std::span{ normal_buffer_data }.subspan(
                    normal_buffer_view.byteOffset + normal_accessor.byteOffset, normal_buffer_view.byteLength
                );
                const auto normal_at = [normal_buffer_data_bytes,
                                        byte_stride = normal_buffer_view.byteStride,
                                        count = normal_accessor.count](std::size_t i) {
                    using element_type = gfx::va_attrib_field<3, float>;
                    ASSERT(i < count);
                    const auto address = byte_stride.value_or(sizeof(element_type)) * i;
                    return *std::bit_cast<const element_type*>(normal_buffer_data_bytes.data() + address);
                };

                const auto tex_coord_attr = primitive.findAttribute("TEXCOORD_0")->accessorIndex;
                const auto& tex_coord_accessor = asset.accessors.at(tex_coord_attr);
                ASSERT(tex_coord_accessor.componentType == ComponentType::Float);
                ASSERT(tex_coord_accessor.type == AccessorType::Vec2);
                ASSERT(tex_coord_accessor.count > 0);
                const auto& tex_coord_buffer_view =
                    asset.bufferViews.at(*ASSERT_VAL(tex_coord_accessor.bufferViewIndex));
                const auto& tex_coord_buffer = asset.buffers.at(tex_coord_buffer_view.bufferIndex);
                const auto& tex_coord_buffer_uri = *ASSERT_VAL(std::get_if<sources::URI>(&tex_coord_buffer.data));
                const auto tex_coord_buffer_data = load_bin(tex_coord_buffer_uri.uri.fspath());
                const auto tex_coord_buffer_data_bytes = std::span{ tex_coord_buffer_data }.subspan(
                    tex_coord_buffer_view.byteOffset + tex_coord_accessor.byteOffset, tex_coord_buffer_view.byteLength
                );
                const auto tex_coord_at = [tex_coord_buffer_data_bytes,
                                           byte_stride = tex_coord_buffer_view.byteStride,
                                           count = tex_coord_accessor.count](std::size_t i) {
                    using element_type = gfx::va_attrib_field<2, float>;
                    ASSERT(i < count);
                    const auto address = byte_stride.value_or(sizeof(element_type)) * i;
                    return *std::bit_cast<const element_type*>(tex_coord_buffer_data_bytes.data() + address);
                };

                const std::vector<VNT> vnts = [&] {
                    const auto vnt_count =
                        std::min(std::min(vert_accessor.count, normal_accessor.count), tex_coord_accessor.count);

                    std::vector<VNT> vnts;
                    vnts.reserve(vnt_count);
                    for (std::uint32_t vnt_i = 0; vnt_i < vnt_count; ++vnt_i) {
                        vnts.push_back(VNT{
                            .vert = vert_at(vnt_i),
                            .normal = normal_at(vnt_i),
                            .tex_coords = tex_coord_at(vnt_i),
                        });
                    }
                    return vnts;
                }();
                ASSERT(!vnts.empty());

                const auto& indices_accessor = asset.accessors.at(*ASSERT_VAL(primitive.indicesAccessor));
                ASSERT(
                    indices_accessor.componentType == ComponentType::UnsignedShort
                    || indices_accessor.componentType == ComponentType::UnsignedInt
                );
                ASSERT(indices_accessor.type == AccessorType::Scalar);
                const auto& indices_buffer_view = asset.bufferViews.at(*ASSERT_VAL(indices_accessor.bufferViewIndex));
                ASSERT(!indices_buffer_view.byteStride.has_value());
                const auto& indices_buffer = asset.buffers.at(indices_buffer_view.bufferIndex);

                const auto f = [&](const auto indices_data) -> exec::async<void> {
                    const std::span indices_span = indices_data.subspan(0, indices_accessor.count);
                    ASSERT(co_await vertex_resource.require(
                        primitive_vertex_id, script::create_vertex(std::span(vnts), indices_span)
                    ));
                };

                const auto& indices_uri = *ASSERT_VAL(std::get_if<sources::URI>(&indices_buffer.data));
                const auto indices_data = load_bin(indices_uri.uri.fspath());
                const auto indicies_data_bytes =
                    std::span{ indices_data }.subspan(indices_buffer_view.byteOffset + indices_accessor.byteOffset);
                if (indices_accessor.componentType == ComponentType::UnsignedShort) {
                    co_await f(std::bit_cast<std::span<const std::uint16_t>>(indicies_data_bytes));
                } else {
                    co_await f(std::bit_cast<std::span<const std::uint32_t>>(indicies_data_bytes));
                }

                ASSERT(co_await primitive_resource.require(
                    primitive_id,
                    exec::value_as_signal(game::primitive{
                        .vtx{ primitive_vertex_id },
                        .mtl{ primitive_material_id },
                    })
                ));
                mesh_asset.primitives.push_back(game::primitive::id{ .id = primitive_id });
                game::log::debug(
                    "primitive_id={} vertex_id={} material_id={}",
                    primitive_id,
                    primitive_vertex_id,
                    primitive_material_id
                );
            }

            co_return mesh_asset;
        }());

        game::log::debug("mesh_id={}", mesh_id);
    }

    const std::vector<entt::entity> node_entities = [&] {
        std::vector<entt::entity> node_entities;
        node_entities.reserve(asset.nodes.size());
        for (std::size_t i = 0; i < asset.nodes.size(); ++i) {
            const entt::entity node_entity = layer.registry.create();
            node_entities.push_back(node_entity);
            game::log::debug("node_entity={}", static_cast<entt::id_type>(node_entity));
        }
        return node_entities;
    }();

    for (const auto& [node_entity, node_document] : ranges::views::zip(node_entities, asset.nodes)) {
        layer.registry.emplace<game::local_transform>(node_entity, [&node = node_document] {
            return node.transform
                   | meta::pmatch{
                         [](const TRS& trs) {
                             return game::transform{
                                 .tr = std::bit_cast<glm::vec3>(trs.translation),
                                 .rot = std::bit_cast<glm::quat>(trs.rotation),
                                 .s = std::bit_cast<glm::vec3>(trs.scale),
                             };
                         },
                         [](const math::fmat4x4& matrix) {
                             return *ASSERT_VAL(game::transform::from_mat4(std::bit_cast<glm::mat4>(matrix)));
                         },
                     };
        }());

        for (const std::size_t child_i : node_document.children) {
            const entt::entity child_entity = node_entities.at(child_i);
            game::node::attach_child(layer, node_entity, child_entity);
        }

        if (node_document.meshIndex.has_value()) {
            const auto mesh_id = "{}.mesh[{}]"_ufs(asset_id, node_document.meshIndex.value())(example_ctx.uss);
            const auto mesh_asset = *ASSERT_VAL(mesh_resource.lookup_unsafe(mesh_id));
            for (const auto& primitive : mesh_asset->primitives) {
                const auto primitive_asset = *ASSERT_VAL(primitive_resource.lookup_unsafe(primitive.id));
                const entt::entity primitive_entity = layer.registry.create();
                layer.registry.emplace<game::shader::id>(primitive_entity, object_shader_id);
                layer.registry.emplace<game::vertex::id>(primitive_entity, primitive_asset->vtx);
                layer.registry.emplace<game::material::id>(primitive_entity, primitive_asset->mtl);
                layer.registry.emplace<game::local_transform>(primitive_entity, game::transform{});
                game::node::attach_child(layer, node_entity, primitive_entity);
            }
            game::log::debug("node_entity={} attached mesh={}", static_cast<entt::id_type>(node_entity), mesh_id);
        }
    }

    const auto& asset_scene_document = asset.scenes.at(*ASSERT_VAL(asset.defaultScene));
    entt::entity imported_scene = layer.registry.create();
    layer.registry.emplace<game::local_transform>(imported_scene, game::transform{});
    for (const auto node_i : asset_scene_document.nodeIndices) {
        const entt::entity node_entity = node_entities.at(node_i);
        game::node::attach_child(layer, imported_scene, node_entity);
    }

    game::log::debug(
        "imported_scene={} with children=[{}]",
        static_cast<entt::id_type>(imported_scene),
        fmt::join(
            layer.registry.get<game::node>(imported_scene).children
                | ranges::views::transform([](entt::entity x) { return static_cast<entt::id_type>(x); }),
            ","
        )
    );
    co_return imported_scene;
}

exec::async<void> create_scene(game::engine_context& e_ctx, ecs::layer& layer, const game::basis& world) {
    auto example_ctx = script::example_context::create(e_ctx);
    // common vvv
    {
        ASSERT(layer.registry.emplace<ecs::resource<game::shader>::ptr_type>(
            layer.root, ecs::resource<game::shader>::make(*e_ctx.sync_exec)
        ));
        ASSERT(layer.registry.emplace<ecs::resource<game::vertex>::ptr_type>(
            layer.root, ecs::resource<game::vertex>::make(*e_ctx.sync_exec)
        ));
        ASSERT(layer.registry.emplace<ecs::resource<game::texture>::ptr_type>(
            layer.root, ecs::resource<game::texture>::make(*e_ctx.sync_exec)
        ));
        ASSERT(layer.registry.emplace<ecs::resource<game::material>::ptr_type>(
            layer.root, ecs::resource<game::material>::make(*e_ctx.sync_exec)
        ));
        ASSERT(layer.registry.emplace<ecs::resource<game::mesh>::ptr_type>(
            layer.root, ecs::resource<game::mesh>::make(*e_ctx.sync_exec)
        ));
        ASSERT(layer.registry.emplace<ecs::resource<game::primitive>::ptr_type>(
            layer.root, ecs::resource<game::primitive>::make(*e_ctx.sync_exec)
        ));

        auto& shader_resource = *layer.registry.get<ecs::resource<game::shader>::ptr_type>(layer.root);
        auto& vertex_resource = *layer.registry.get<ecs::resource<game::vertex>::ptr_type>(layer.root);
        auto& texture_resource = *layer.registry.get<ecs::resource<game::texture>::ptr_type>(layer.root);
        auto& material_resource = *layer.registry.get<ecs::resource<game::material>::ptr_type>(layer.root);
        ASSERT(co_await vertex_resource.require(
            "vertex.cube"_us(example_ctx.uss), script::create_vertex(std::span(cube_vertices), std::span(cube_indices))
        ));
        ASSERT(co_await shader_resource.require(
            "shader.object"_us(example_ctx.uss), create_object_shader(example_ctx, world)
        ));
        ASSERT(co_await shader_resource.require("shader.unlit"_us(example_ctx.uss), create_unlit_shader(example_ctx)));
        auto texture_diffuse = *ASSERT_VAL(co_await texture_resource.require(
            "texture.diffuse"_us(example_ctx.uss),
            script::create_texture(example_ctx.examples_path / "textures/03_lightmap_diffuse.png")
        ));
        auto texture_specular = *ASSERT_VAL(co_await texture_resource.require(
            "texture.specular"_us(example_ctx.uss),
            script::create_texture(example_ctx.examples_path / "textures/03_lightmap_specular.png")
        ));
        ASSERT(co_await material_resource.require(
            "material.crate"_us(example_ctx.uss),
            exec::value_as_signal(game::material{
                .diffuse = std::move(texture_diffuse),
                .specular = std::move(texture_specular),
                .shininess = 128.0f * 0.6f,
            })
        ));
        ASSERT(co_await material_resource.require(
            "material.solid"_us(example_ctx.uss),
            exec::value_as_signal(game::material{
                .diffuse = glm::vec4{ 1.0f },
                .specular = glm::vec4{ 0.0f },
                .shininess = 128.0f * 0.6f,
            })
        ));
    }
    // common ^^^

    const auto global_entity = co_await create_global_entity(e_ctx, example_ctx, layer);
    const auto player_entity = co_await create_player_entity(e_ctx, layer, world);
    game::node::attach_child(layer, global_entity, player_entity);

    const auto light_entities = co_await create_light_entities(e_ctx, example_ctx, layer);
    game::node::attach_children(layer, global_entity, std::span{ light_entities });

    const auto cube_entities = co_await create_cube_entities(example_ctx, layer, world);
    game::node::attach_children(layer, global_entity, std::span{ cube_entities });

    const entt::entity imported_entity = co_await create_imported_entity(example_ctx, layer, "meshes/cube.gltf");
    game::node::attach_child(layer, global_entity, imported_entity);
}

void main(int argc, char** argv) {
    auto a_logger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    game::logger().set_level(spdlog::level::debug);
    game::logger().sinks().push_back(a_logger);
    gfx::logger().set_level(spdlog::level::debug);
    gfx::logger().sinks().push_back(a_logger);

    auto w_ctx = *ASSERT_VAL(
        game::window_context::initialize(sl::meta::null, "04_many_lights", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f })
    );
    auto e_ctx = game::engine_context::initialize(std::move(w_ctx), argc, argv);
    ecs::layer layer{};
    game::graphics_system gfx_system{ .layer = layer, .world{} };
    game::overlay_system overlay_system{ .layer = layer };

    exec::coro_schedule(*e_ctx.script_exec, create_scene(e_ctx, layer, gfx_system.world));

    while (e_ctx.is_ok()) {
        e_ctx.spin_once(layer, gfx_system, overlay_system);
    }

    while (e_ctx.script_exec->execute_batch() > 0) {}
}

} // namespace sl

int main(int argc, char** argv) {
    sl::main(argc, argv);
    return 0;
}
