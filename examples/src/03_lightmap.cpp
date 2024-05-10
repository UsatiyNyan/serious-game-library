//
// Created by usatiynyan.
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

template <typename... Args>
using uniform_setter = fu2::unique_function<void(const gfx::bound_shader_program&, Args...)>;

struct object_shader {
    gfx::shader_program sp;

    uniform_setter<const float*> set_model;
    uniform_setter<const float*> set_it_model;
    uniform_setter<const float*> set_transform;

    struct material {
        // diffuse
        // specular
        // emission
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
    uniform_setter<float> set_time;
};

gfx::texture<gfx::texture_type::texture_2d> create_texture(const std::filesystem::path& image_path) {
    gfx::texture_builder<gfx::texture_type::texture_2d> tex_builder;
    tex_builder.set_wrap_s(gfx::texture_wrap::repeat);
    tex_builder.set_wrap_t(gfx::texture_wrap::repeat);
    tex_builder.set_min_filter(gfx::texture_filter::nearest);
    tex_builder.set_max_filter(gfx::texture_filter::nearest);

    const auto image = *ASSERT_VAL(stb::image_load(image_path, 4));
    tex_builder.set_image(std::span{ image.dimensions }, gfx::texture_format{ GL_RGB, GL_RGBA }, image.data.get());
    return std::move(tex_builder).submit();
}

object_shader create_object_shader(const std::filesystem::path& root) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/03_lightmap.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/03_lightmap.frag")),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();

    constexpr std::array<std::string_view, 3> material_textures{
        "u_material.diffuse",
        "u_material.specular",
        "u_material.emission",
    };
    sp_bind.initialize_tex_units(std::span{ material_textures });

    return object_shader{
        .sp = std::move(sp),
        .set_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_model", 1, false)),
        .set_it_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix3fv, "u_it_model", 1, false)),
        .set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false)),
        .material{
            .set_shininess = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1f, "u_material.shininess")),
        },
        .light{
            .set_position = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.position")),
            .set_ambient = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.ambient")),
            .set_diffuse = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.diffuse")),
            .set_specular = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light.specular")),
        },
        .set_view_pos = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_view_pos")),
        .set_time = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1f, "u_time")),
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

struct VNT {
    gfx::va_attrib_field<3, float> vert;
    gfx::va_attrib_field<3, float> normal;
    gfx::va_attrib_field<2, float> tex_coords;
};

template <std::size_t vertices_extent, std::size_t indices_extent>
auto create_buffers(std::span<const VNT, vertices_extent> vnts, std::span<const unsigned, indices_extent> indices) {
    gfx::vertex_array_builder va_builder;
    va_builder.attributes_from<VNT>();
    auto vb = va_builder.buffer<gfx::buffer_type::array, gfx::buffer_usage::static_draw>(vnts);
    auto eb = va_builder.buffer<gfx::buffer_type::element_array, gfx::buffer_usage::static_draw>(indices);
    auto va = std::move(va_builder).submit();
    return std::make_tuple(std::move(vb), std::move(eb), std::move(va));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
constexpr std::array cube_vertices{
    // verts                 | normals            | tex coords
    // front face
    VNT{ +0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +0.0f, +1.0f }, // top left
    // right face
    VNT{ +0.5f, +0.5f, +0.5f, +1.0f, +0.0f, +0.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, -0.5f, +0.5f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    VNT{ +0.5f, -0.5f, -0.5f, +1.0f, +0.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    VNT{ +0.5f, +0.5f, -0.5f, +1.0f, +0.0f, +0.0f, +0.0f, +1.0f }, // top left
    // back face
    VNT{ +0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +0.0f, +1.0f }, // top left
    // left face
    VNT{ -0.5f, +0.5f, -0.5f, -1.0f, +0.0f, +0.0f, +1.0f, +1.0f }, // top right
    VNT{ -0.5f, -0.5f, -0.5f, -1.0f, +0.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, -0.5f, +0.5f, -1.0f, +0.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, +0.5f, +0.5f, -1.0f, +0.0f, +0.0f, +0.0f, +1.0f }, // top left
    // top face
    VNT{ +0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f }, // top left
    // bottom face
    VNT{ +0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f, +1.0f, +1.0f }, // top right
    VNT{ +0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    VNT{ -0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    VNT{ -0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f, +0.0f, +1.0f }, // top left
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

struct render_object {
    glm::vec3 position;
    struct material {
        gfx::texture<gfx::texture_type::texture_2d> diffuse;
        gfx::texture<gfx::texture_type::texture_2d> specular;
        gfx::texture<gfx::texture_type::texture_2d> emission;
        float shininess;
    };
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
    const sl::rt::context rt_ctx{ argc, argv };
    const auto root = rt_ctx.path().parent_path();

    spdlog::set_level(spdlog::level::info);

    game::graphics graphics =
        *ASSERT_VAL(game::initialize_graphics("03_lightmap", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f }));

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
                .diffuse = create_texture(root / "textures/03_lightmap_diffuse.png"),
                .specular = create_texture(root / "textures/03_lightmap_specular.png"),
                .emission = create_texture(root / "textures/03_lightmap_emission.jpg"),
                .shininess = 128.0f * 0.6f,
            },
        },
        render_object{
            .position{ 5.0f, 0.0f, 0.0f },
            .material{
                .diffuse = create_texture(root / "textures/03_lightmap_diffuse.png"),
                .specular = create_texture(root / "textures/03_lightmap_specular.png"),
                .emission = create_texture(root / "textures/03_lightmap_emission.jpg"),
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

    game::time time;

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
        const game::time_point time_point = time.calculate();
        const auto tr = calculate_tr(kis);
        const auto maybe_cursor_offset = calculate_cursor_offset(mis, cis);
        camera_update(render.camera, time_point.delta_sec(), tr, maybe_cursor_offset);

        // render
        auto bound_render = render.bind(graphics.current_window, *graphics.state);

        // source
        {
            const auto& [vb, eb, va] = source_buffers;
            const auto& [sp, set_transform, set_light_color] = source_shader;

            gfx::draw draw{ sp.bind(), va };

            set_light_color(draw.sp(), light.ambient.r, light.ambient.g, light.ambient.b);
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position);
            const glm::mat4 transform = bound_render.projection * bound_render.view * model;
            set_transform(draw.sp(), glm::value_ptr(transform));
            draw.elements(eb);
        }

        // objects
        {
            auto bound_sp = object_shader.sp.bind();
            object_shader.light.set_position(bound_sp, light.position.x, light.position.y, light.position.z);
            object_shader.light.set_ambient(bound_sp, light.ambient.r, light.ambient.g, light.ambient.b);
            object_shader.light.set_diffuse(bound_sp, light.diffuse.r, light.diffuse.g, light.diffuse.b);
            object_shader.light.set_specular(bound_sp, light.specular.r, light.specular.g, light.specular.b);
            const auto& view_position = render.camera.tf.tr;
            object_shader.set_view_pos(bound_sp, view_position.x, view_position.y, view_position.z);
            object_shader.set_time(bound_sp, time_point.now_sec().count());

            for (const auto& [position, material] : cube_objects) {
                const auto& [vb, eb, va] = object_buffers;
                gfx::draw draw{ std::move(bound_sp), va, material.diffuse, material.specular, material.emission };

                object_shader.material.set_shininess(draw.sp(), material.shininess);

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
                object_shader.set_model(draw.sp(), glm::value_ptr(model));
                object_shader.set_it_model(draw.sp(), glm::value_ptr(it_model));
                object_shader.set_transform(draw.sp(), glm::value_ptr(transform));
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
                ImGui::SliderFloat("shininess", &material.shininess, 2.0f, 256, "%.0f", ImGuiSliderFlags_Logarithmic);
                ImGui::PopID();
            }
        }
    }

    return 0;
}
