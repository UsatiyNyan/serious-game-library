//
// Created by usatiynyan.
//

#include <imgui.h>
#include <sl/game.hpp>
#include <sl/gfx.hpp>
#include <sl/meta.hpp>
#include <sl/rt.hpp>

#include <libassert/assert.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <spdlog/spdlog.h>

#include <stb/image.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

namespace game = sl::game;
namespace engine = sl::game::engine;
namespace rt = sl::rt;
using sl::meta::operator""_hsv;

namespace sl::game {

struct directional_light {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct point_light {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct spot_light {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
    float cutoff;
    float outer_cutoff;
};

// alignment due to (3) https://registry.khronos.org/OpenGL/specs/gl/glspec46.core.pdf#page=168
struct directional_light_buffer_element {
    alignas(16) glm::vec3 direction;

    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;

    static directional_light_buffer_element from(const basis& world, const transform& tf, const directional_light& dl) {
        return {
            .direction = world.direction(tf),
            .ambient = dl.ambient,
            .diffuse = dl.diffuse,
            .specular = dl.specular,
        };
    }
};

struct point_light_buffer_element {
    alignas(16) glm::vec3 position;

    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;

    static point_light_buffer_element from(const basis&, const transform& tf, const point_light& pl) {
        return {
            .position = tf.tr,
            .ambient = pl.ambient,
            .diffuse = pl.diffuse,
            .specular = pl.specular,
            .constant = pl.constant,
            .linear = pl.linear,
            .quadratic = pl.quadratic,
        };
    }
};

struct spot_light_buffer_element {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 direction;

    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
    float cutoff;
    float outer_cutoff;

    static spot_light_buffer_element from(const basis& world, const transform& tf, const spot_light& sl) {
        return {
            .position = tf.tr,
            .direction = world.direction(tf),
            .ambient = sl.ambient,
            .diffuse = sl.diffuse,
            .specular = sl.specular,
            .constant = sl.constant,
            .linear = sl.linear,
            .quadratic = sl.quadratic,
            .cutoff = sl.cutoff,
            .outer_cutoff = sl.outer_cutoff,
        };
    }
};

template <typename T>
auto init_ssbo(std::size_t size) {
    gfx::buffer<T, gfx::buffer_type::shader_storage, gfx::buffer_usage::dynamic_draw> ssbo;
    ssbo.bind().initialize_data(size);
    return ssbo;
}

template <typename ComponentView, typename T>
std::uint32_t set_ssbo_data(
    const basis& world,
    ComponentView component_view,
    gfx::buffer<T, gfx::buffer_type::shader_storage, gfx::buffer_usage::dynamic_draw>& ssbo
) {
    std::uint32_t size_counter = 0;
    auto bound_ssbo = ssbo.bind();
    auto maybe_mapped_ssbo = bound_ssbo.template map<gfx::buffer_access::write_only>();
    auto mapped_ssbo = *ASSERT_VAL(std::move(maybe_mapped_ssbo));
    auto mapped_ssbo_data = mapped_ssbo.data();
    for (const auto& [entity, tf, elem] : component_view.each()) {
        if (size_counter == mapped_ssbo_data.size()) {
            spdlog::warn("exceeded limit of components: {}", mapped_ssbo_data.size());
            break;
        }
        mapped_ssbo_data[size_counter] = T::from(world, tf, elem);
        ++size_counter;
    }
    return size_counter;
};

texture create_texture(const std::filesystem::path& image_path) {
    gfx::texture_builder tex_builder{ gfx::texture_type::texture_2d };
    tex_builder.set_wrap_s(gfx::texture_wrap::repeat);
    tex_builder.set_wrap_t(gfx::texture_wrap::repeat);
    tex_builder.set_min_filter(gfx::texture_filter::nearest);
    tex_builder.set_max_filter(gfx::texture_filter::nearest);

    const auto image = *ASSERT_VAL(stb::image_load(image_path, 4));
    tex_builder.set_image(std::span{ image.dimensions }, gfx::texture_format{ GL_RGB, GL_RGBA }, image.data.get());
    return texture{ .tex = std::move(tex_builder).submit() };
}

shader<engine::layer> create_object_shader(const std::filesystem::path& root) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/05_object.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/05_object.frag")),
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

    auto set_material_shininess = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform1f, "u_material.shininess"));

    auto set_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_model", 1, false));
    auto set_it_model = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix3fv, "u_it_model", 1, false));
    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));

    auto dl_buffer = init_ssbo<directional_light_buffer_element>(1);
    auto pl_buffer = init_ssbo<point_light_buffer_element>(16);
    auto sl_buffer = init_ssbo<spot_light_buffer_element>(1);

    return shader<engine::layer>{
        .sp{ std::move(sp) },
        .setup{ [ //
                    set_view_pos = std::move(set_view_pos),
                    dl_buffer = std::move(dl_buffer),
                    set_dl_size = std::move(set_directional_light_size),
                    pl_buffer = std::move(pl_buffer),
                    set_pl_size = std::move(set_point_light_size),
                    sl_buffer = std::move(sl_buffer),
                    set_sl_size = std::move(set_spot_light_size),
                    set_material_shininess = std::move(set_material_shininess),
                    set_model = std::move(set_model),
                    set_it_model = std::move(set_it_model),
                    set_transform = std::move(set_transform)](
                    engine::layer& layer, //
                    const game::camera_frame& camera_frame,
                    const gfx::bound_shader_program& bound_sp
                ) mutable {
            set_view_pos(bound_sp, camera_frame.position.x, camera_frame.position.y, camera_frame.position.z);

            const std::uint32_t dl_size =
                set_ssbo_data(layer.world, layer.registry.view<transform, directional_light>(), dl_buffer);
            set_dl_size(bound_sp, dl_size);

            const std::uint32_t pl_size =
                set_ssbo_data(layer.world, layer.registry.view<transform, point_light>(), pl_buffer);
            set_pl_size(bound_sp, pl_size);

            const std::uint32_t sl_size =
                set_ssbo_data(layer.world, layer.registry.view<transform, spot_light>(), sl_buffer);
            set_sl_size(bound_sp, sl_size);

            return [ //
                       &set_material_shininess,
                       &set_model,
                       &set_it_model,
                       &set_transform,
                       &layer,
                       &camera_frame,
                       &bound_sp,
                       bound_dl_base = dl_buffer.bind_base(0),
                       bound_pl_base = pl_buffer.bind_base(1),
                       bound_sl_base = sl_buffer.bind_base(2)]( //
                       const gfx::bound_vertex_array& bound_va,
                       vertex::draw_type& vertex_draw,
                       std::span<const entt::entity> entities
                   ) {
                for (const entt::entity entity : entities) {
                    if (const bool hass = layer.registry.all_of<transform, material::id>(entity); !ASSUME_VAL(hass)) {
                        continue;
                    }

                    const auto& tf = layer.registry.get<transform>(entity);
                    const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), tf.tr);
                    const glm::mat4 rotation_matrix = glm::mat4_cast(tf.rot);
                    // TODO: scale
                    const glm::mat4 model = translation_matrix * rotation_matrix;
                    const glm::mat3 it_model = glm::transpose(glm::inverse(model));
                    const glm::mat4 transform = camera_frame.projection * camera_frame.view * model;
                    set_model(bound_sp, glm::value_ptr(model));
                    set_it_model(bound_sp, glm::value_ptr(it_model));
                    set_transform(bound_sp, glm::value_ptr(transform));

                    const auto& mtl_id = layer.registry.get<material::id>(entity);
                    const auto mtl = *ASSERT_VAL(layer.storage.material.lookup(mtl_id.id));
                    const std::array texs{ mtl->diffuse, mtl->specular };
                    const auto bound_texs = gfx::activate_textures(
                        texs | ranges::views::transform([](const auto& x) -> const gfx::texture& { return x->tex; })
                    );
                    set_material_shininess(bound_sp, mtl->shininess);

                    gfx::draw draw{ bound_sp, bound_va, bound_texs };
                    vertex_draw(draw);
                }
            };
        } },
    };
}

shader<engine::layer> create_source_shader(const std::filesystem::path& root) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/05_source.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/05_source.frag")),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();
    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));
    auto set_light_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light_color"));
    return shader<engine::layer>{
        .sp = std::move(sp),
        .setup{
            [ //
                set_transform = std::move(set_transform),
                set_light_color = std::move(set_light_color)]( //
                engine::layer& layer,
                const game::camera_frame& camera_frame,
                const gfx::bound_shader_program& bound_sp
            ) {
                return [ //
                           &set_transform,
                           &set_light_color,
                           &layer,
                           &camera_frame,
                           &bound_sp]( //
                           const gfx::bound_vertex_array& bound_va,
                           vertex::draw_type& vertex_draw,
                           std::span<const entt::entity> entities
                       ) {
                    for (const entt::entity entity : entities) {
                        if (const bool hass = layer.registry.all_of<transform, point_light>(entity);
                            !ASSUME_VAL(hass)) {
                            continue;
                        }

                        const auto& tf = layer.registry.get<transform>(entity);
                        const glm::mat4 transform =
                            camera_frame.projection * camera_frame.view * glm::translate(glm::mat4(1.0f), tf.tr);
                        set_transform(bound_sp, glm::value_ptr(transform));

                        const auto& pl = layer.registry.get<point_light>(entity);
                        const glm::vec3& color = pl.ambient;
                        set_light_color(bound_sp, color.r, color.g, color.b);

                        gfx::draw draw{ bound_sp, bound_va };
                        vertex_draw(draw);
                    }
                };
            },
        },
    };
}

struct VNT {
    gfx::va_attrib_field<3, float> vert;
    gfx::va_attrib_field<3, float> normal;
    gfx::va_attrib_field<2, float> tex_coords;
};

template <std::size_t vertices_extent, std::size_t indices_extent>
vertex
    create_vertex(std::span<const VNT, vertices_extent> vnts, std::span<const std::uint32_t, indices_extent> indices) {
    gfx::vertex_array_builder va_builder;
    va_builder.attributes_from<VNT>();
    auto vb = va_builder.buffer<gfx::buffer_type::array, gfx::buffer_usage::static_draw>(vnts);
    auto eb = va_builder.buffer<gfx::buffer_type::element_array, gfx::buffer_usage::static_draw>(indices);
    return vertex{
        .va = std::move(va_builder).submit(),
        .draw{ [vb = std::move(vb), eb = std::move(eb)](gfx::draw& draw) { draw.elements(eb); } },
    };
}

struct global_entity_state {
    meta::dirty<bool> should_close;
};

struct player_entity_state {
    bool q = false;
    bool w = false;
    bool e = false;
    bool a = false;
    bool s = false;
    bool d = false;
    meta::dirty<bool> is_rotating;
    tl::optional<glm::dvec2> cursor_prev;
    glm::dvec2 cursor_curr;
};

} // namespace sl::game


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
constexpr std::array cube_vertices{
    // verts                 | normals            | tex coords
    // front face
    game::VNT{ +0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +1.0f, +1.0f }, // top right
    game::VNT{ +0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +1.0f, +0.0f }, // bottom right
    game::VNT{ -0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f }, // bottom left
    game::VNT{ -0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +0.0f, +1.0f }, // top left
    // right face
    game::VNT{ +0.5f, +0.5f, +0.5f, +1.0f, +0.0f, +0.0f, +1.0f, +1.0f }, // top right
    game::VNT{ +0.5f, -0.5f, +0.5f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    game::VNT{ +0.5f, -0.5f, -0.5f, +1.0f, +0.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    game::VNT{ +0.5f, +0.5f, -0.5f, +1.0f, +0.0f, +0.0f, +0.0f, +1.0f }, // top left
    // back face
    game::VNT{ +0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +1.0f, +1.0f }, // top right
    game::VNT{ +0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +1.0f, +0.0f }, // bottom right
    game::VNT{ -0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f }, // bottom left
    game::VNT{ -0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +0.0f, +1.0f }, // top left
    // left face
    game::VNT{ -0.5f, +0.5f, -0.5f, -1.0f, +0.0f, +0.0f, +1.0f, +1.0f }, // top right
    game::VNT{ -0.5f, -0.5f, -0.5f, -1.0f, +0.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    game::VNT{ -0.5f, -0.5f, +0.5f, -1.0f, +0.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    game::VNT{ -0.5f, +0.5f, +0.5f, -1.0f, +0.0f, +0.0f, +0.0f, +1.0f }, // top left
    // top face
    game::VNT{ +0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f, +1.0f, +1.0f }, // top right
    game::VNT{ +0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    game::VNT{ -0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    game::VNT{ -0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f }, // top left
    // bottom face
    game::VNT{ +0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f, +1.0f, +1.0f }, // top right
    game::VNT{ +0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f, +1.0f, +0.0f }, // bottom right
    game::VNT{ -0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f, +0.0f, +0.0f }, // bottom left
    game::VNT{ -0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f, +0.0f, +1.0f }, // top left
};
#pragma GCC diagnostic pop

constexpr std::array cube_indices{
    0u,  1u,  3u,  1u,  2u,  3u, // Front face
    4u,  5u,  7u,  5u,  6u,  7u, // Right face
    8u,  9u,  11u, 9u,  10u, 11u, // Back face
    12u, 13u, 15u, 13u, 14u, 15u, // Left face
    16u, 17u, 19u, 17u, 18u, 19u, // Top face
    20u, 21u, 23u, 21u, 22u, 23u, // Bottom face
};

constexpr std::array cube_positions{
    glm::vec3{ 0.0f, 0.0f, 0.0f }, //
    glm::vec3{ 0.0f, 0.0f, 2.0f }, //
    glm::vec3{ 0.0f, 0.0f, 4.0f }, //
    glm::vec3{ 0.0f, 0.0f, 6.0f }, //
    glm::vec3{ 0.0f, 0.0f, 8.0f }, //
    glm::vec3{ 2.0f, 0.0f, 0.0f }, //
    glm::vec3{ 2.0f, 0.0f, 2.0f }, //
    glm::vec3{ 2.0f, 0.0f, 4.0f }, //
    glm::vec3{ 2.0f, 0.0f, 6.0f }, //
    glm::vec3{ 2.0f, 0.0f, 8.0f }, //
    glm::vec3{ 4.0f, 0.0f, 0.0f }, //
    glm::vec3{ 4.0f, 0.0f, 2.0f }, //
    glm::vec3{ 4.0f, 0.0f, 4.0f }, //
    glm::vec3{ 4.0f, 0.0f, 6.0f }, //
    glm::vec3{ 4.0f, 0.0f, 8.0f }, //
    glm::vec3{ 6.0f, 0.0f, 0.0f }, //
    glm::vec3{ 6.0f, 0.0f, 2.0f }, //
    glm::vec3{ 6.0f, 0.0f, 4.0f }, //
    glm::vec3{ 6.0f, 0.0f, 6.0f }, //
    glm::vec3{ 6.0f, 0.0f, 8.0f }, //
    glm::vec3{ 8.0f, 0.0f, 0.0f }, //
    glm::vec3{ 8.0f, 0.0f, 2.0f }, //
    glm::vec3{ 8.0f, 0.0f, 4.0f }, //
    glm::vec3{ 8.0f, 0.0f, 6.0f }, //
    glm::vec3{ 8.0f, 0.0f, 8.0f }, //
};

constexpr std::array point_light_positions{
    glm::vec3{ 10.0f, 0.0f, 0.0f },
    glm::vec3{ 0.0f, 10.0f, 0.0f },
    glm::vec3{ 0.0f, 0.0f, 10.0f },
};

constexpr std::array point_lights{
    game::point_light{
        .ambient{ 1.0f, 0.0f, 0.0f },
        .diffuse{ 1.0f, 0.0f, 0.0f },
        .specular{ 1.0f, 0.0f, 0.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
    game::point_light{
        .ambient{ 0.0f, 1.0f, 0.0f },
        .diffuse{ 0.0f, 1.0f, 0.0f },
        .specular{ 1.0f, 1.0f, 1.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
    game::point_light{
        .ambient{ 0.0f, 0.0f, 1.0f },
        .diffuse{ 0.0f, 0.0f, 1.0f },
        .specular{ 0.0f, 0.0f, 1.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
};

constexpr game::directional_light global_directional_light{
    .ambient{ 0.05f, 0.05f, 0.05f },
    .diffuse{ 0.4f, 0.4f, 0.4f },
    .specular{ 0.5f, 0.5f, 0.5f },
};

static const game::spot_light player_spot_light{
    .ambient{ 0.0f, 0.0f, 0.0f },
    .diffuse{ 1.0f, 1.0f, 1.0f },
    .specular{ 1.0f, 1.0f, 1.0f },

    .constant = 1.0f,
    .linear = 0.09f,
    .quadratic = 0.032f,

    .cutoff = glm::cos(glm::radians(12.5f)),
    .outer_cutoff = glm::cos(glm::radians(15.0f)),
};

int main(int argc, char** argv) {
    spdlog::set_level(spdlog::level::debug);

    engine::context e_ctx =
        game::window_context::initialize("05_gfx_system", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f })
            .map([&](auto&& w_ctx) { return engine::context::initialize(std::move(w_ctx), argc, argv); })
            .or_else([] { PANIC("engine context"); })
            .value();

    engine::layer layer{ *e_ctx.script_exec, engine::layer::config{} };
    const auto object_shader_id = *ASSERT_VAL(layer.storage.string.emplace("object.shader"_hsv));
    ASSERT(layer.storage.shader.insert(object_shader_id, game::create_object_shader(e_ctx.root_path)));

    const auto source_shader_id = *ASSERT_VAL(layer.storage.string.emplace("source.shader"_hsv));
    ASSERT(layer.storage.shader.insert(source_shader_id, game::create_source_shader(e_ctx.root_path)));

    const auto cube_vertex_id = *ASSERT_VAL(layer.storage.string.emplace("cube.vertex"_hsv));
    const auto cube_texture_diffuse_id = *ASSERT_VAL(layer.storage.string.emplace("cube.texture.diffuse"_hsv));
    const auto cube_texture_specular_id = *ASSERT_VAL(layer.storage.string.emplace("cube.texture.specular"_hsv));
    const auto cube_material_id = *ASSERT_VAL(layer.storage.string.emplace("cube.material"_hsv));
    ASSERT(layer.storage.vertex.insert(
        cube_vertex_id, game::create_vertex(std::span{ cube_vertices }, std::span{ cube_indices })
    ));
    const auto cube_texture_diffuse = *ASSERT_VAL(layer.storage.texture.insert(
        cube_texture_diffuse_id, game::create_texture(e_ctx.root_path / "textures/03_lightmap_diffuse.png")
    ));
    const auto cube_texture_specular = *ASSERT_VAL(layer.storage.texture.insert(
        cube_texture_specular_id, game::create_texture(e_ctx.root_path / "textures/03_lightmap_specular.png")
    ));
    ASSERT(layer.storage.material.insert(
        cube_material_id,
        game::material{
            .diffuse = cube_texture_diffuse,
            .specular = cube_texture_specular,
            .shininess = 128.0f * 0.6f,
        }
    ));

    const auto object_entities = //
        ranges::views::enumerate(cube_positions) | ranges::views::transform([&](auto ip) {
            auto [i, p] = ip;
            const float angle = 20.0f * (static_cast<float>(i));
            const entt::entity entity = layer.registry.create();
            layer.registry.emplace<game::shader<engine::layer>::id>(entity, object_shader_id);
            layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
            layer.registry.emplace<game::material::id>(entity, cube_material_id);
            layer.registry.emplace<game::transform>(
                entity,
                game::transform{
                    .tr = p,
                    .rot = glm::angleAxis(glm::radians(angle), layer.world.up()),
                }
            );
            return entity;
        })
        | ranges::to<std::vector>();
    const sl::meta::defer destroy_object_entities{ [&] {
        layer.registry.destroy(object_entities.begin(), object_entities.end());
    } };

    const auto point_light_entities = //
        ranges::views::zip(point_light_positions, point_lights) | ranges::views::transform([&](auto pl) {
            auto [p, l] = pl;
            const entt::entity entity = layer.registry.create();
            layer.registry.emplace<game::shader<engine::layer>::id>(entity, source_shader_id);
            layer.registry.emplace<game::vertex::id>(entity, cube_vertex_id);
            layer.registry.emplace<game::point_light>(entity, l);
            layer.registry.emplace<game::transform>(entity, game::transform{ .tr = p, .rot{} });
            return entity;
        })
        | ranges::to<std::vector>();
    const sl::meta::defer destroy_point_light_entities{ [&] {
        layer.registry.destroy(point_light_entities.begin(), point_light_entities.end());
    } };

    const entt::entity global_entity = [&] {
        const entt::entity entity = layer.root;
        layer.registry.emplace<game::global_entity_state>(entity);
        layer.registry.emplace<game::directional_light>(entity, global_directional_light);
        auto rot = glm::rotation(layer.world.forward(), glm::normalize(glm::vec3{ -0.2f, -1.0f, -0.3f }));
        layer.registry.emplace<game::transform>(entity, game::transform{ .tr{}, .rot{ rot } });
        layer.registry.emplace<game::input<engine::layer>>(
            entity,
            [](engine::layer& layer, const game::input_events& input_events, entt::entity entity) {
                using action = game::keyboard_input_event::action_type;
                auto& state = *ASSERT_VAL((layer.registry.try_get<game::global_entity_state>(entity)));
                const sl::meta::pmatch handle{
                    [&state](const game::keyboard_input_event& keyboard) {
                        using key = game::keyboard_input_event::key_type;
                        if (keyboard.key == key::ESCAPE) {
                            const bool prev = state.should_close.get().value_or(false);
                            state.should_close.set_if_ne(prev || keyboard.action == action::PRESS);
                        }
                    },
                    [](const auto&) {},
                };
                for (const auto& input_event : input_events) {
                    handle(input_event);
                }
            }
        );
        layer.registry.emplace<game::update<engine::layer>>(
            entity,
            [&](engine::layer& layer, entt::entity entity, rt::time_point) {
                auto& state = *ASSERT_VAL((layer.registry.try_get<game::global_entity_state>(entity)));
                state.should_close.release().map([&cw = e_ctx.w_ctx.current_window](bool should_close) {
                    cw.set_should_close(should_close);
                });
            }
        );
        return entity;
    }();
    const sl::meta::defer destroy_global_entity{ [&] { layer.registry.destroy(global_entity); } };

    const entt::entity player_entity = [&] {
        const entt::entity entity = layer.registry.create();
        layer.registry.emplace<game::player_entity_state>(entity);
        layer.registry.emplace<game::spot_light>(entity, player_spot_light);
        layer.registry.emplace<game::transform>(
            entity,
            game::transform{
                .tr = glm::vec3{ 0.0f, 0.0f, 3.0f },
                .rot = glm::angleAxis(glm::radians(-180.0f), layer.world.up()),
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
        layer.registry.emplace<game::input<engine::layer>>(
            entity,
            [](engine::layer& layer, const game::input_events& input_events, entt::entity entity) {
                using action = game::keyboard_input_event::action_type;
                auto& state = *ASSERT_VAL((layer.registry.try_get<game::player_entity_state>(entity)));
                const sl::meta::pmatch handle{
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
                            state.is_rotating.set_if_ne(
                                mouse_button.action == action::PRESS || mouse_button.action == action::REPEAT
                            );
                        }
                    },
                    [&state](const game::cursor_input_event& cursor) {
                        // protection against multiple cursor events between updates
                        if (!state.cursor_prev.has_value()) {
                            state.cursor_prev.emplace(state.cursor_curr);
                        }
                        state.cursor_curr = cursor.pos;
                    },
                };
                for (const auto& input_event : input_events) {
                    handle(input_event);
                }
            }
        );
        layer.registry.emplace<game::update<engine::layer>>(
            entity,
            [&e_ctx](engine::layer& layer, entt::entity entity, rt::time_point time_point) {
                if (const bool hass = layer.registry.all_of<game::player_entity_state, game::transform>(entity);
                    !ASSUME_VAL(hass)) {
                    return;
                }

                auto& state = layer.registry.get<game::player_entity_state>(entity);
                auto& tf = layer.registry.get<game::transform>(entity);

                if (const sl::meta::defer consume{ [&state] { state.cursor_prev.reset(); } };
                    state.cursor_prev.has_value() && state.is_rotating.get().value_or(false)) {
                    const auto cursor_offset = state.cursor_curr - state.cursor_prev.value();

                    constexpr float sensitivity = -glm::radians(0.2f);
                    const float yaw = static_cast<float>(cursor_offset.x) * sensitivity;
                    const float pitch = static_cast<float>(cursor_offset.y) * sensitivity;

                    const glm::quat rot_yaw = glm::angleAxis(yaw, layer.world.y);
                    tf.rotate(rot_yaw);

                    const glm::vec3 camera_forward = tf.rot * layer.world.forward();
                    const glm::vec3 camera_right = glm::cross(camera_forward, layer.world.up());
                    const glm::quat rot_pitch = glm::angleAxis(pitch, camera_right);
                    tf.rotate(rot_pitch);
                }

                {
                    constexpr float acc = 5.0f;
                    const float speed = acc * time_point.delta_sec().count();
                    const auto new_tr = [&state, &world = layer.world] {
                        constexpr auto sub = [](bool a, bool b) {
                            return static_cast<float>(static_cast<int>(a) - static_cast<int>(b));
                        };
                        return sub(state.e, state.q) * world.up() + //
                               sub(state.d, state.a) * world.right() + //
                               sub(state.w, state.s) * world.forward();
                    }();
                    tf.translate(speed * (tf.rot * new_tr));
                }

                state.is_rotating.release().map([&e_ctx](bool is_rotating) {
                    e_ctx.w_ctx.current_window.set_input_mode(
                        GLFW_CURSOR, is_rotating ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
                    );
                });
            }
        );

        return entity;
    }();
    const sl::meta::defer destroy_player_entity{ [&] { layer.registry.destroy(player_entity); } };

    while (!e_ctx.w_ctx.current_window.should_close()) {
        // input
        e_ctx.w_ctx.context->poll_events();
        e_ctx.w_ctx.state->frame_buffer_size.release().map( //
            [&cw = e_ctx.w_ctx.current_window](const glm::ivec2& frame_buffer_size) {
                cw.viewport(glm::ivec2{}, frame_buffer_size);
            }
        );
        e_ctx.w_ctx.state->window_content_scale.release().map([](const glm::fvec2& window_content_scale) {
            ImGui::GetStyle().ScaleAllSizes(window_content_scale.x);
        });
        e_ctx.in_sys->process(layer);

        // update
        const rt::time_point time_point = e_ctx.time.calculate();
        game::update_system(layer, time_point);

        // render
        const auto window_frame = e_ctx.w_ctx.new_frame();
        game::graphics_system(layer, window_frame);

        // overlay
        auto imgui_frame = e_ctx.w_ctx.imgui.new_frame(); // TODO: require gfx_frame here too
        if (const auto imgui_window = imgui_frame.begin("debug")) {
            // TODO: maybe use actual angles against the world basis
            auto& directional_light_rot = layer.registry.get<game::transform>(global_entity).rot;
            ImGui::SliderFloat4("directional_light rot", glm::value_ptr(directional_light_rot), -1.0f, 1.0f);
            directional_light_rot = glm::normalize(directional_light_rot);

            auto& directional_light = layer.registry.get<game::directional_light>(global_entity);
            ImGui::ColorEdit3("directional_light ambient", glm::value_ptr(directional_light.ambient));
            ImGui::ColorEdit3("directional_light diffuse", glm::value_ptr(directional_light.diffuse));
            ImGui::ColorEdit3("directional_light specular", glm::value_ptr(directional_light.specular));

            ImGui::Spacing();
            layer.storage.material.lookup(cube_material_id).map([](sl::meta::persistent<game::material> material) {
                ImGui::SliderFloat("shininess", &material->shininess, 2.0f, 256, "%.0f", ImGuiSliderFlags_Logarithmic);
            });
        }
    }

    return 0;
}
