//
// Created by usatiynyan.
//

#include <sl/game.hpp>
#include <sl/gfx.hpp>
#include <sl/rt.hpp>

#include <libassert/assert.hpp>
#include <spdlog/spdlog.h>

#include <stb/image.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <sl/meta/lifetime/defer.hpp>

namespace game = sl::game;
namespace gfx = sl::gfx;

auto create_object_shader(const std::filesystem::path& root) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/01_lighting_object.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/01_lighting_object.frag")),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();
    auto set_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_model", 1, false));
    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));
    auto set_light_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light_color"));
    auto set_light_pos = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light_pos"));
    auto set_object_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_object_color"));
    return std::make_tuple(
        std::move(sp),
        std::move(set_model),
        std::move(set_transform),
        std::move(set_light_color),
        std::move(set_light_pos),
        std::move(set_object_color)
    );
};

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
};

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
};

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

constexpr std::array cube_positions{
    glm::vec3{ 0.0f, 0.0f, 0.0f }, //
};

int main(int argc, char** argv) {
    const sl::rt::context rt_ctx{ argc, argv };
    const auto root = rt_ctx.path().parent_path();

    spdlog::set_level(spdlog::level::info);

    game::graphics graphics =
        *ASSERT_VAL(game::initialize_graphics("01_lighting", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f }));

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

    // TODO: generic_input
    constexpr auto is_esc_pressed = [](const gfx::current_window& cw) { return cw.is_key_pressed(GLFW_KEY_ESCAPE); };
    const auto poll_movement = [&world = render.world](const gfx::current_window& cw) {
        gfx::transform movement{};
        if (cw.is_key_pressed(GLFW_KEY_W)) {
            movement.translate(world.forward());
        }
        if (cw.is_key_pressed(GLFW_KEY_S)) {
            movement.translate(-world.forward());
        }
        if (cw.is_key_pressed(GLFW_KEY_D)) {
            movement.translate(world.right());
        }
        if (cw.is_key_pressed(GLFW_KEY_A)) {
            movement.translate(-world.right());
        }
        if (cw.is_key_pressed(GLFW_KEY_Q)) {
            movement.translate(-world.up());
        }
        if (cw.is_key_pressed(GLFW_KEY_E)) {
            movement.translate(world.up());
        }
        return normalize(movement);
    };

    const auto update = [&camera =
                             render.camera](std::chrono::duration<float> delta_time, const gfx::transform& movement) {
        constexpr float camera_acc = 2.5f;
        const float camera_speed = camera_acc * delta_time.count();
        camera.tf.translate(camera_speed * (camera.tf.rot * movement.tr));
    };

    std::optional<glm::dvec2> last_cursor_pos{};

    // TODO: dirty
    (void)graphics.window->mouse_button_cb.connect([&](int button, int action, int mods [[maybe_unused]]) {
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            const int new_input_mode = action == GLFW_PRESS ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
            graphics.current_window.set_input_mode(GLFW_CURSOR, new_input_mode);
            last_cursor_pos.reset();
        }
    });

    // TODO: dirty
    (void)graphics.window->cursor_pos_cb.connect([&](glm::dvec2 cursor_pos) {
        if (graphics.current_window.get_input_mode(GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
            return;
        }

        if (!last_cursor_pos.has_value()) {
            last_cursor_pos = cursor_pos;
            return;
        }
        const glm::vec2 cursor_offset = *last_cursor_pos - cursor_pos;
        last_cursor_pos = cursor_pos;

        constexpr float sensitivity = glm::radians(0.1f);
        const float yaw = cursor_offset.x * sensitivity;
        const float pitch = cursor_offset.y * sensitivity;

        const glm::quat rot_yaw = glm::angleAxis(yaw, render.world.y);
        render.camera.tf.rotate(rot_yaw);

        const glm::vec3 camera_forward = render.camera.tf.rot * render.world.forward();
        const glm::vec3 camera_right = glm::cross(camera_forward, render.world.up());
        const glm::quat rot_pitch = glm::angleAxis(pitch, camera_right);
        render.camera.tf.rotate(rot_pitch);
    });

    // prepare render
    const auto object_shader = create_object_shader(root);
    const auto object_buffers = create_buffers(std::span{ cube_vertices }, std::span{ indices });

    // TODO: many lights?
    glm::vec3 source_position{ 2.0f, 2.0f, -3.0f };
    const auto source_shader = create_source_shader(root);
    const auto source_buffers = create_buffers(std::span{ cube_vertices }, std::span{ indices });
    glm::vec3 source_color{ 1.0f, 1.0f, 1.0f };

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
        if (is_esc_pressed(graphics.current_window)) {
            graphics.current_window.set_should_close(true);
        }
        const gfx::transform movement = poll_movement(graphics.current_window);

        // time
        const std::chrono::duration<float> delta_time = time.calculate_delta();

        // update
        update(delta_time, movement);

        // render
        auto bound_render = render.bind(graphics.current_window, *graphics.state);

        // source
        {
            const auto& [vb, eb, va] = source_buffers;
            const auto& [sp, set_transform, set_light_color] = source_shader;

            gfx::draw draw{ sp, va, {} };

            set_light_color(draw.sp(), source_color.r, source_color.g, source_color.b);

            const glm::mat4 model = glm::translate(glm::mat4(1.0f), source_position);
            const glm::mat4 transform = bound_render.projection * bound_render.view * model;
            set_transform(draw.sp(), glm::value_ptr(transform));
            draw.elements(eb);
        }

        // objects
        {
            const auto& [vb, eb, va] = object_buffers;
            const auto& [sp, set_model, set_transform, set_light_color, set_light_pos, set_object_color] =
                object_shader;

            gfx::draw draw{ sp, va, {} };

            set_light_color(draw.sp(), source_color.r, source_color.g, source_color.b);
            set_light_pos(draw.sp(), source_position.x, source_position.y, source_position.z);
            set_object_color(draw.sp(), 0.61f, 0.08f, 0.90f); // #9c15e6

            for (const auto& pos : cube_positions) {
                const glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
                set_model(draw.sp(), glm::value_ptr(model));
                const glm::mat4 transform = bound_render.projection * bound_render.view * model;
                set_transform(draw.sp(), glm::value_ptr(transform));
                draw.elements(eb);
            }
        }

        // overlay
        graphics.imgui.new_frame();
        sl::meta::defer imgui_render{ [&imgui = graphics.imgui] { imgui.render(); } };

        if (const sl::meta::defer imgui_end{ ImGui::End }; ImGui::Begin("light")) {
            ImGui::SliderFloat3("light pos", glm::value_ptr(source_position), -10.0f, 10.0f);
            ImGui::ColorEdit3("light color", glm::value_ptr(source_color));
        }
    }

    return 0;
}
