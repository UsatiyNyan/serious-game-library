//
// Created by usatiynyan.
// TODO: fix this
//

#include <imgui.h>
#include <sl/game.hpp>
#include <sl/gfx.hpp>
#include <sl/meta.hpp>
#include <sl/rt.hpp>

#include <libassert/assert.hpp>
#include <spdlog/spdlog.h>

#include <stb/image.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

namespace game = sl::game;
namespace gfx = sl::gfx;
namespace rt = sl::rt;

template <typename... Args>
using uniform_setter = fu2::unique_function<void(const gfx::bound_shader_program&, Args...)>;

struct object_shader {
    gfx::shader_program sp;

    uniform_setter<const float*> set_model;
    uniform_setter<const float*> set_it_model;
    uniform_setter<const float*> set_transform;

    struct material {
        uniform_setter<float, float, float> set_ambient;
        uniform_setter<float, float, float> set_diffuse;
        uniform_setter<float, float, float> set_specular;
        uniform_setter<float> set_shininess;
    };
    material material;

    struct light {
        uniform_setter<float, float, float> set_position;
        uniform_setter<float, float, float> set_ambient;
        uniform_setter<float, float, float> set_diffuse;
        uniform_setter<float, float, float> set_specular;
    };
    light light;

    uniform_setter<float, float, float> set_view_pos;
};

object_shader create_object_shader(const std::filesystem::path& root) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/02_material_object.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/02_material_object.frag")),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();
    return object_shader{
        .sp = std::move(sp),
        .set_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_model", 1, false)),
        .set_it_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix3fv, "u_it_model", 1, false)),
        .set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false)),
        .material{
            .set_ambient = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_material.ambient")),
            .set_diffuse = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_material.diffuse")),
            .set_specular = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_material.specular")),
            .set_shininess = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1f, "u_material.shininess")),
        },
        .light{
            .set_position = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.position")),
            .set_ambient = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.ambient")),
            .set_diffuse = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.diffuse")),
            .set_specular = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.specular")),
        },
        .set_view_pos = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_view_pos")),
    };
}

auto create_source_shader(const std::filesystem::path& root) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/01_lighting_source.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/01_lighting_source.frag")),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();
    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));
    auto set_light_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light_color"));
    return std::make_tuple(std::move(sp), std::move(set_transform), std::move(set_light_color));
}

struct VN {
    gfx::va_attrib_field<3, float> vert;
    gfx::va_attrib_field<3, float> normal;
};

template <std::size_t vertices_extent, std::size_t indices_extent>
auto create_buffers(std::span<const VN, vertices_extent> vns, std::span<const unsigned, indices_extent> indices) {
    gfx::vertex_array_builder va_builder;
    va_builder.attributes_from<VN>();
    auto vb = va_builder.buffer<gfx::buffer_type::array, gfx::buffer_usage::static_draw>(vns);
    auto eb = va_builder.buffer<gfx::buffer_type::element_array, gfx::buffer_usage::static_draw>(indices);
    auto va = std::move(va_builder).submit();
    return std::make_tuple(std::move(vb), std::move(eb), std::move(va));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
constexpr std::array cube_vertices{
    // verts            | normals
    // front face
    VN{ +0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f }, // top right
    VN{ +0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f }, // bottom right
    VN{ -0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f }, // bottom left
    VN{ -0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f }, // top left
    // right face
    VN{ +0.5f, +0.5f, +0.5f, +1.0f, +0.0f, +0.0f }, // top right
    VN{ +0.5f, -0.5f, +0.5f, +1.0f, +0.0f, +0.0f }, // bottom right
    VN{ +0.5f, -0.5f, -0.5f, +1.0f, +0.0f, +0.0f }, // bottom left
    VN{ +0.5f, +0.5f, -0.5f, +1.0f, +0.0f, +0.0f }, // top left
    // back face
    VN{ +0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f }, // top right
    VN{ +0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f }, // bottom right
    VN{ -0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f }, // bottom left
    VN{ -0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f }, // top left
    // left face
    VN{ -0.5f, +0.5f, -0.5f, -1.0f, +0.0f, +0.0f }, // top right
    VN{ -0.5f, -0.5f, -0.5f, -1.0f, +0.0f, +0.0f }, // bottom right
    VN{ -0.5f, -0.5f, +0.5f, -1.0f, +0.0f, +0.0f }, // bottom left
    VN{ -0.5f, +0.5f, +0.5f, -1.0f, +0.0f, +0.0f }, // top left
    // top face
    VN{ +0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f }, // top right
    VN{ +0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f }, // bottom right
    VN{ -0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f }, // bottom left
    VN{ -0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f }, // top left
    // bottom face
    VN{ +0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f }, // top right
    VN{ +0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f }, // bottom right
    VN{ -0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f }, // bottom left
    VN{ -0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f }, // top left
};
#pragma GCC diagnostic pop

constexpr std::array indices{
    0u,  1u,  3u,  1u,  2u,  3u, // Front face
    4u,  5u,  7u,  5u,  6u,  7u, // Right face
    8u,  9u,  11u, 9u,  10u, 11u, // Back face
    12u, 13u, 15u, 13u, 14u, 15u, // Left face
    16u, 17u, 19u, 17u, 18u, 19u, // Top face
    20u, 21u, 23u, 21u, 22u, 23u, // Bottom face
};

struct material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

struct render_object {
    glm::vec3 position;
    material material;
};

struct light {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct keys_input_state {
    sl::meta::dirty<bool> esc{ false };
    sl::meta::dirty<bool> q{ false };
    sl::meta::dirty<bool> w{ false };
    sl::meta::dirty<bool> e{ false };
    sl::meta::dirty<bool> a{ false };
    sl::meta::dirty<bool> s{ false };
    sl::meta::dirty<bool> d{ false };
};

struct mouse_input_state {
    sl::meta::dirty<bool> right{ false };
};

struct cursor_input_state {
    sl::meta::dirty<glm::dvec2> last;
    sl::meta::dirty<glm::dvec2> curr;
};

int main(int argc, char** argv) {
    const rt::context rt_ctx{ argc, argv };
    const auto root = rt_ctx.path().parent_path();

    spdlog::set_level(spdlog::level::info);

    game::graphics graphics =
        *ASSERT_VAL(game::graphics::initialize("02_material", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f }));

    keys_input_state kis;
    (void)graphics.window->key_cb.connect([&kis](int key, int /* scancode */, int action, int /* mods */) {
        const bool is_pressed = action == GLFW_PRESS || action == GLFW_REPEAT;
        switch (key) {
        case GLFW_KEY_ESCAPE:
            kis.esc.set(is_pressed);
            break;
        case GLFW_KEY_Q:
            kis.q.set(is_pressed);
            break;
        case GLFW_KEY_W:
            kis.w.set(is_pressed);
            break;
        case GLFW_KEY_E:
            kis.e.set(is_pressed);
            break;
        case GLFW_KEY_A:
            kis.a.set(is_pressed);
            break;
        case GLFW_KEY_S:
            kis.s.set(is_pressed);
            break;
        case GLFW_KEY_D:
            kis.d.set(is_pressed);
            break;
        }
    });

    mouse_input_state mis;
    (void)graphics.window->mouse_button_cb.connect([&mis](int button, int action, int mods [[maybe_unused]]) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            mis.right.set(action == GLFW_PRESS || action == GLFW_REPEAT);
        }
    });

    cursor_input_state cis;
    (void)graphics.window->cursor_pos_cb.connect([&cis](glm::dvec2 cursor_pos) { cis.curr.set(cursor_pos); });

    constexpr auto calculate_tr = [](const keys_input_state& kis) {
        constexpr auto sub = [](bool a, bool b) {
            return static_cast<float>(static_cast<int>(a) - static_cast<int>(b));
        };
        constexpr gfx::basis local;
        return sub(kis.e.get(), kis.q.get()) * local.up() + //
               sub(kis.d.get(), kis.a.get()) * local.right() + //
               sub(kis.w.get(), kis.s.get()) * local.forward();
    };

    constexpr auto calculate_cursor_offset = [](const mouse_input_state& mis,
                                                cursor_input_state& cis) -> tl::optional<glm::dvec2> {
        if (!mis.right.get()) {
            cis.curr.then([&cis](glm::dvec2 curr) { cis.last.set(curr); });
            return tl::nullopt;
        }
        return cis.curr
            .then([&cis](const glm::dvec2& curr) {
                auto offset = cis.last.then([&curr](const glm::dvec2& last) { return curr - last; });
                cis.last.set(curr);
                return offset;
            })
            .value_or(tl::nullopt);
    };

    constexpr auto camera_update = [](gfx::camera& camera,
                                      std::chrono::duration<float> delta_time,
                                      const glm::vec3& tr,
                                      const tl::optional<glm::dvec2>& maybe_cursor_offset) {
        maybe_cursor_offset.map([&camera](const glm::dvec2& cursor_offset) {
            constexpr float sensitivity = -glm::radians(0.1f);
            const float yaw = static_cast<float>(cursor_offset.x) * sensitivity;
            const float pitch = static_cast<float>(cursor_offset.y) * sensitivity;

            constexpr gfx::basis local;
            const glm::quat rot_yaw = glm::angleAxis(yaw, local.y);
            camera.tf.rotate(rot_yaw);

            const glm::vec3 camera_forward = camera.tf.rot * local.forward();
            const glm::vec3 camera_right = glm::cross(camera_forward, local.up());
            const glm::quat rot_pitch = glm::angleAxis(pitch, camera_right);
            camera.tf.rotate(rot_pitch);
        });

        constexpr float camera_acc = 2.5f;
        const float camera_speed = camera_acc * delta_time.count();
        camera.tf.translate(camera_speed * (camera.tf.rot * tr));
    };

    // prepare render
    game::render render{
        graphics,
        gfx::perspective_projection{
            .fov = glm::radians(45.0f),
            .near = 0.1f,
            .far = 100.0f,
        },
    };
    render.camera.tf = gfx::transform{
        .tr = glm::vec3{ 0.0f, 0.0f, 3.0f },
        .rot = glm::angleAxis(glm::radians(-180.0f), render.world.up()),
    };

    auto object_shader = create_object_shader(root);
    const auto object_buffers = create_buffers(std::span{ cube_vertices }, std::span{ indices });

    // TODO: many lights?
    const auto source_shader = create_source_shader(root);
    const auto source_buffers = create_buffers(std::span{ cube_vertices }, std::span{ indices });

    std::array cube_objects{
        render_object{
            .position{ 0.0f, 0.0f, 0.0f },
            .material{
                .ambient{ 0.1745f, 0.01175f, 0.01175f },
                .diffuse{ 0.61424f, 0.04136f, 0.04136f },
                .specular{ 0.727811f, 0.626959f, 0.626959f },
                .shininess = 128.0f * 0.6f,
            },
        },
        render_object{
            .position{ 5.0f, 0.0f, 0.0f },
            .material{
                .ambient{ 1.0f, 0.5f, 0.31f },
                .diffuse{ 1.0f, 0.5f, 0.31f },
                .specular{ 0.5f, 0.5f, 0.5f },
                .shininess = 32.0f,
            },
        },
    };

    light light{
        .position{ 2.0f, 2.0f, -3.0f },
        .ambient{ 0.2f, 0.2f, 0.2f },
        .diffuse{ 0.5f, 0.5f, 0.5f },
        .specular{ 1.0f, 1.0f, 1.0f },
    };

    rt::time time;

    while (!graphics.current_window.should_close()) {
        // input
        graphics.context->poll_events();

        // process input
        graphics.state->frame_buffer_size.then([&cw = graphics.current_window](glm::ivec2 frame_buffer_size) {
            cw.viewport(glm::ivec2{}, frame_buffer_size);
        });
        graphics.state->window_content_scale.then([](glm::fvec2 window_content_scale) {
            ImGui::GetStyle().ScaleAllSizes(window_content_scale.x);
        });
        kis.esc.then([&cw = graphics.current_window](bool esc) { cw.set_should_close(esc); });
        mis.right.then([&cw = graphics.current_window](bool right) {
            cw.set_input_mode(GLFW_CURSOR, right ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        });

        // update
        const std::chrono::duration<float> delta_time = time.calculate().delta_sec();
        const auto tr = calculate_tr(kis);
        const auto maybe_cursor_offset = calculate_cursor_offset(mis, cis);
        camera_update(render.camera, delta_time, tr, maybe_cursor_offset);

        // render
        auto bound_render = render.bind(graphics.current_window, *graphics.state);

        // source
        {
            const auto& [vb, eb, va] = source_buffers;
            const auto& [sp, set_transform, set_light_color] = source_shader;

            const auto bound_sp = sp.bind();
            const auto bound_va = va.bind();
            gfx::draw draw{ bound_sp, bound_va };

            set_light_color(bound_sp, light.ambient.r, light.ambient.g, light.ambient.b);
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position);
            const glm::mat4 transform = bound_render.projection * bound_render.view * model;
            set_transform(bound_sp, glm::value_ptr(transform));
            draw.elements(eb);
        }

        // objects
        {
            const auto& [vb, eb, va] = object_buffers;
            const auto bound_sp = object_shader.sp.bind();
            const auto bound_va = va.bind();
            gfx::draw draw{ bound_sp, bound_va };

            const auto& view_position = render.camera.tf.tr;
            object_shader.set_view_pos(bound_sp, view_position.x, view_position.y, view_position.z);

            object_shader.light.set_position(bound_sp, light.position.x, light.position.y, light.position.z);
            object_shader.light.set_ambient(bound_sp, light.ambient.r, light.ambient.g, light.ambient.b);
            object_shader.light.set_diffuse(bound_sp, light.diffuse.r, light.diffuse.g, light.diffuse.b);
            object_shader.light.set_specular(bound_sp, light.specular.r, light.specular.g, light.specular.b);

            for (const auto& [position, material] : cube_objects) {
                object_shader.material.set_ambient(
                    bound_sp, material.ambient.r, material.ambient.g, material.ambient.b
                );
                object_shader.material.set_diffuse(
                    bound_sp, material.diffuse.r, material.diffuse.g, material.diffuse.b
                );
                object_shader.material.set_specular(
                    bound_sp, material.specular.r, material.specular.g, material.specular.b
                );
                object_shader.material.set_shininess(bound_sp, material.shininess);

                const auto make_model =
                    sl::meta::pipeline{} //
                        .then([&p = position](auto x) { return glm::translate(x, p); })
                        .then([&render](auto x) { return glm::rotate(x, glm::radians(60.0f), render.world.up()); });
                const auto make_it_model = sl::meta::pipeline{} //
                                               .then(SL_META_LIFT(glm::inverse))
                                               .then(SL_META_LIFT(glm::transpose));

                const glm::mat4 model = make_model(glm::mat4(1.0f));
                const glm::mat3 it_model = make_it_model(model);
                const glm::mat4 transform = bound_render.projection * bound_render.view * model;
                object_shader.set_model(bound_sp, glm::value_ptr(model));
                object_shader.set_it_model(bound_sp, glm::value_ptr(it_model));
                object_shader.set_transform(bound_sp, glm::value_ptr(transform));
                draw.elements(eb);
            }
        }

        // overlay
        auto imgui_frame = graphics.imgui.new_frame();
        if (const auto imgui_window = imgui_frame.begin("light")) {
            ImGui::SliderFloat3("light position", glm::value_ptr(light.position), -10.0f, 10.0f);
            ImGui::ColorEdit3("light ambient", glm::value_ptr(light.ambient));
            ImGui::ColorEdit3("light diffuse", glm::value_ptr(light.diffuse));
            ImGui::ColorEdit3("light specular", glm::value_ptr(light.specular));

            int i = 0;
            for (auto& [_, material] : cube_objects) {
                ImGui::PushID(i++);
                ImGui::Spacing();
                ImGui::ColorEdit3("ambient", glm::value_ptr(material.ambient));
                ImGui::ColorEdit3("diffuse", glm::value_ptr(material.diffuse));
                ImGui::ColorEdit3("specular", glm::value_ptr(material.specular));
                ImGui::SliderFloat("shininess", &material.shininess, 2.0f, 256, "%.0f", ImGuiSliderFlags_Logarithmic);
                ImGui::PopID();
            }
        }
    }

    return 0;
}
