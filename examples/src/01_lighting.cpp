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
    uniform_setter<float, float, float> set_light_color;
    uniform_setter<float, float, float> set_light_pos;
    uniform_setter<float, float, float> set_view_pos;
    uniform_setter<float, float, float> set_object_color;
    uniform_setter<float> set_ambient_strength;
    uniform_setter<float> set_specular_strength;
    uniform_setter<int> set_shininess;
};

object_shader create_object_shader(const std::filesystem::path& root) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/01_lighting_object.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/01_lighting_object.frag")),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();
    return object_shader{
        .sp = std::move(sp),
        .set_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_model", 1, false)),
        .set_it_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix3fv, "u_it_model", 1, false)),
        .set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false)),
        .set_light_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light_color")),
        .set_light_pos = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light_pos")),
        .set_view_pos = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_view_pos")),
        .set_object_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_object_color")),
        .set_ambient_strength = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1f, "u_ambient_strength")),
        .set_specular_strength = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1f, "u_specular_strength")),
        .set_shininess = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1i, "u_shininess")),
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

struct CubeObject {
    glm::vec3 pos;
    glm::vec3 color;
};

constexpr std::array cube_objects{
    CubeObject{ glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.61f, 0.08f, 0.90f } }, //
    CubeObject{ glm::vec3{ 5.0f, 0.0f, 0.0f }, glm::vec3{ 0.61f, 0.00f, 0.31f } }, //
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
        *ASSERT_VAL(game::initialize_graphics("01_lighting", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f }));

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
    glm::vec3 source_position{ 2.0f, 2.0f, -3.0f };
    const auto source_shader = create_source_shader(root);
    const auto source_buffers = create_buffers(std::span{ cube_vertices }, std::span{ indices });
    glm::vec3 source_color{ 1.0f, 1.0f, 1.0f };
    float ambient_strength = 0.1f;
    float specular_strength = 0.5f;
    int shininess = 32;

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
        const std::chrono::duration<float> delta_time = time.calculate_delta();
        const auto tr = calculate_tr(kis);
        const auto maybe_cursor_offset = calculate_cursor_offset(mis, cis);
        camera_update(render.camera, delta_time, tr, maybe_cursor_offset);

        // render
        auto bound_render = render.bind(graphics.current_window, *graphics.state);

        // source
        {
            const auto& [vb, eb, va] = source_buffers;
            const auto& [sp, set_transform, set_light_color] = source_shader;

            gfx::draw draw{ sp.bind(), va };

            set_light_color(draw.sp(), source_color.r, source_color.g, source_color.b);
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), source_position);
            const glm::mat4 transform = bound_render.projection * bound_render.view * model;
            set_transform(draw.sp(), glm::value_ptr(transform));
            draw.elements(eb);
        }

        // objects
        {
            const auto& [vb, eb, va] = object_buffers;
            gfx::draw draw{ object_shader.sp.bind(), va };

            object_shader.set_light_color(draw.sp(), source_color.r, source_color.g, source_color.b);
            object_shader.set_light_pos(draw.sp(), source_position.x, source_position.y, source_position.z);
            const auto& view_position = render.camera.tf.tr;
            object_shader.set_view_pos(draw.sp(), view_position.x, view_position.y, view_position.z);
            object_shader.set_ambient_strength(draw.sp(), ambient_strength);
            object_shader.set_specular_strength(draw.sp(), specular_strength);
            object_shader.set_shininess(draw.sp(), shininess);

            for (const auto& cube : cube_objects) {
                object_shader.set_object_color(draw.sp(), cube.color.r, cube.color.g, cube.color.b);
                const auto make_model =
                    sl::meta::pipeline{} //
                        .then([&pos = cube.pos](auto x) { return glm::translate(x, pos); })
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
            ImGui::SliderFloat3("light pos", glm::value_ptr(source_position), -10.0f, 10.0f);
            ImGui::ColorEdit3("light color", glm::value_ptr(source_color));
            ImGui::SliderFloat("ambient strength", &ambient_strength, 0.0f, 1.0f);
            ImGui::SliderFloat("specular strength", &specular_strength, 0.0f, 1.0f);
            ImGui::SliderInt("shininess", &shininess, 2, 256, "%d", ImGuiSliderFlags_Logarithmic);
        }
    }

    return 0;
}
