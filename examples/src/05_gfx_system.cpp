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

namespace sl::game {

struct directional_light_component {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct point_light_component {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct spot_light_component {
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

    static directional_light_buffer_element
        from_component(const gfx::basis& world, const transform_component& tf, const directional_light_component& dl) {
        return {
            .direction = tf.direction(world),
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

    static point_light_buffer_element
        from_component(const gfx::basis&, const transform_component& tf, const point_light_component& pl) {
        return {
            .position = tf.tf.tr,
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

    static spot_light_buffer_element
        from_component(const gfx::basis& world, const transform_component& tf, const spot_light_component& sl) {
        return {
            .position = tf.tf.tr,
            .direction = tf.direction(world),
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
    const gfx::basis& world,
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
        mapped_ssbo_data[size_counter] = T::from_component(world, tf, elem);
        ++size_counter;
    }
    return size_counter;
};

struct material_component {
    struct id {
        meta::unique_string id;
    };

    meta::persistent<texture_component> diffuse;
    meta::persistent<texture_component> specular;
    float shininess;
};

texture_component create_texture_component(const std::filesystem::path& image_path) {
    gfx::texture_builder tex_builder{ gfx::texture_type::texture_2d };
    tex_builder.set_wrap_s(gfx::texture_wrap::repeat);
    tex_builder.set_wrap_t(gfx::texture_wrap::repeat);
    tex_builder.set_min_filter(gfx::texture_filter::nearest);
    tex_builder.set_max_filter(gfx::texture_filter::nearest);

    const auto image = *ASSERT_VAL(stb::image_load(image_path, 4));
    tex_builder.set_image(std::span{ image.dimensions }, gfx::texture_format{ GL_RGB, GL_RGBA }, image.data.get());
    return texture_component{ .tex = std::move(tex_builder).submit() };
}

struct layer {
    struct {
        meta::unique_string_storage string;
        storage<shader_component<layer>> shader;
        storage<vertex_component> vertex;
        storage<texture_component> texture;
        storage<material_component> material;
    } storage;
    entt::registry registry;
};

shader_component<layer> create_object_shader_component(const std::filesystem::path& root) {
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

    return shader_component<layer>{
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
                    const game::camera_state& camera_state, //
                    const gfx::bound_shader_program& bound_sp,
                    layer& layer
                ) mutable {
            set_view_pos(bound_sp, camera_state.position.x, camera_state.position.y, camera_state.position.z);

            const std::uint32_t dl_size = set_ssbo_data(
                camera_state.world, layer.registry.view<transform_component, directional_light_component>(), dl_buffer
            );
            set_dl_size(bound_sp, dl_size);

            const std::uint32_t pl_size = set_ssbo_data(
                camera_state.world, layer.registry.view<transform_component, point_light_component>(), pl_buffer
            );
            set_pl_size(bound_sp, pl_size);

            const std::uint32_t sl_size = set_ssbo_data(
                camera_state.world, layer.registry.view<transform_component, spot_light_component>(), sl_buffer
            );
            set_sl_size(bound_sp, sl_size);

            return [ //
                       &set_material_shininess,
                       &set_model,
                       &set_it_model,
                       &set_transform,
                       &camera_state,
                       &bound_sp,
                       &layer,
                       bound_dl_base = dl_buffer.bind_base(0),
                       bound_pl_base = pl_buffer.bind_base(1),
                       bound_sl_base = sl_buffer.bind_base(2)]( //
                       const gfx::bound_vertex_array& bound_va,
                       vertex_component::draw_type& vertex_draw,
                       std::span<const entt::entity> entities
                   ) {
                for (const entt::entity entity : entities) {
                    if (const bool has_components =
                            layer.registry.all_of<transform_component, material_component::id>(entity);
                        !ASSUME_VAL(has_components)) {
                        continue;
                    }

                    const auto& tf = layer.registry.get<transform_component>(entity);
                    const auto make_model = meta::pipeline{} //
                                                .then([&tr = tf.tf.tr](auto x) { return glm::translate(x, tr); })
                                                .then([rot = glm::toMat4(tf.tf.rot)](auto x) { return rot * x; });
                    constexpr auto make_it_model = meta::pipeline{} //
                                                       .then(SL_META_LIFT(glm::inverse))
                                                       .then(SL_META_LIFT(glm::transpose));
                    const glm::mat4 model = make_model(glm::mat4(1.0f));
                    const glm::mat3 it_model = make_it_model(model);
                    const glm::mat4 transform = camera_state.projection * camera_state.view * model;
                    set_model(bound_sp, glm::value_ptr(model));
                    set_it_model(bound_sp, glm::value_ptr(it_model));
                    set_transform(bound_sp, glm::value_ptr(transform));

                    const auto& mtl_id = layer.registry.get<material_component::id>(entity);
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

shader_component<layer> create_source_shader_component(const std::filesystem::path& root) {
    const std::array<gfx::shader, 2> shaders{
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::vertex, root / "shaders/05_source.vert")),
        *ASSERT_VAL(gfx::shader::load_from_file(gfx::shader_type::fragment, root / "shaders/05_source.frag")),
    };
    auto sp = *ASSERT_VAL(gfx::shader_program::build(std::span{ shaders }));
    auto sp_bind = sp.bind();
    auto set_transform = *ASSERT_VAL(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));
    auto set_light_color = *ASSERT_VAL(sp_bind.make_uniform_setter(glUniform3f, "u_light_color"));
    return shader_component<layer>{
        .sp = std::move(sp),
        .setup{
            [ //
                set_transform = std::move(set_transform),
                set_light_color = std::move(set_light_color)]( //
                const game::camera_state& camera_state,
                const gfx::bound_shader_program& bound_sp,
                layer& layer
            ) {
                return [ //
                           &set_transform,
                           &set_light_color,
                           &camera_state,
                           &bound_sp,
                           &layer]( //
                           const gfx::bound_vertex_array& bound_va,
                           vertex_component::draw_type& vertex_draw,
                           std::span<const entt::entity> entities
                       ) {
                    for (const entt::entity entity : entities) {
                        if (const bool has_components =
                                layer.registry.all_of<transform_component, point_light_component>(entity);
                            !ASSUME_VAL(has_components)) {
                            continue;
                        }

                        const auto& tf_component = layer.registry.get<transform_component>(entity);
                        const glm::mat4 transform = camera_state.projection * camera_state.view
                                                    * glm::translate(glm::mat4(1.0f), tf_component.tf.tr);
                        set_transform(bound_sp, glm::value_ptr(transform));

                        const auto& pl_component = layer.registry.get<point_light_component>(entity);
                        const glm::vec3& color = pl_component.ambient;
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
vertex_component create_vertex_component(
    std::span<const VNT, vertices_extent> vnts,
    std::span<const std::uint32_t, indices_extent> indices
) {
    gfx::vertex_array_builder va_builder;
    va_builder.attributes_from<VNT>();
    auto vb = va_builder.buffer<gfx::buffer_type::array, gfx::buffer_usage::static_draw>(vnts);
    auto eb = va_builder.buffer<gfx::buffer_type::element_array, gfx::buffer_usage::static_draw>(indices);
    return vertex_component{
        .va = std::move(va_builder).submit(),
        .draw{ [vb = std::move(vb), eb = std::move(eb)](gfx::draw& draw) { draw.elements(eb); } },
    };
}

} // namespace sl::game

namespace game = sl::game;
namespace gfx = sl::gfx;
namespace rt = sl::rt;

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
    glm::vec3{ 2.0f, 5.0f, -15.0f }, //
    glm::vec3{ -1.5f, -2.2f, -2.5f }, //
    glm::vec3{ -3.8f, -2.0f, -12.3f }, //
    glm::vec3{ 2.4f, -0.4f, -3.5f }, //
    glm::vec3{ -1.7f, 3.0f, -7.5f }, //
    glm::vec3{ 1.3f, -2.0f, -2.5f }, //
    glm::vec3{ 1.5f, 2.0f, -2.5f }, //
    glm::vec3{ 1.5f, 0.2f, -1.5f }, //
    glm::vec3{ -1.3f, 1.0f, -1.5f }, //
};

constexpr std::array point_light_positions{
    glm::vec3{ 0.7f, 0.2f, 2.0f },
    glm::vec3{ 2.3f, -3.3f, -4.0f },
    glm::vec3{ -4.0f, 2.0f, -12.0f },
    glm::vec3{ 0.0f, 0.0f, -3.0f },
};

constexpr std::array point_light_components{
    game::point_light_component{
        .ambient{ 0.05f, 0.0f, 0.0f },
        .diffuse{ 0.8f, 0.0f, 0.0f },
        .specular{ 1.0f, 0.0f, 0.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
    game::point_light_component{
        .ambient{ 0.0f, 0.0f, 0.05f },
        .diffuse{ 0.0f, 0.0f, 0.8f },
        .specular{ 0.0f, 0.0f, 1.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
    game::point_light_component{
        .ambient{ 0.05f, 0.05f, 0.05f },
        .diffuse{ 0.8f, 0.8f, 0.8f },
        .specular{ 1.0f, 1.0f, 1.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
    game::point_light_component{
        .ambient{ 0.05f, 0.05f, 0.05f },
        .diffuse{ 0.8f, 0.8f, 0.8f },
        .specular{ 1.0f, 1.0f, 1.0f },

        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
    },
};

constexpr game::directional_light_component global_directional_light_component{
    .ambient{ 0.05f, 0.05f, 0.05f },
    .diffuse{ 0.4f, 0.4f, 0.4f },
    .specular{ 0.5f, 0.5f, 0.5f },
};

static const game::spot_light_component player_spot_light_component{
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

    const rt::context rt_ctx{ argc, argv };
    const auto root = rt_ctx.path().parent_path();

    game::graphics gfx =
        *ASSERT_VAL(game::graphics::initialize("05_gfx_system", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f }));
    game::graphics_system gfx_system;

    game::input_system input_system{ *gfx.window };

    game::layer layer{
        .storage{
            .string{ 128 },
            .shader{ 2 },
            .vertex{ 1 },
            .texture{ 2 },
            .material{ 1 },
        },
        .registry{},
    };

    constexpr auto check = [](auto&& p) {
        auto [value, is_emplaced] = p;
        ASSUME(is_emplaced);
        return value;
    };

    using sl::meta::operator""_hsv;

    const auto object_shader_id = check(layer.storage.string.emplace("object.shader"_hsv));
    check( //
        layer.storage.shader.emplace(object_shader_id, [&root] { return game::create_object_shader_component(root); })
    );

    const auto source_shader_id = check(layer.storage.string.emplace("source.shader"_hsv));
    check( //
        layer.storage.shader.emplace(source_shader_id, [&root] { return game::create_source_shader_component(root); })
    );

    const auto cube_vertex_id = check(layer.storage.string.emplace("cube.vertex"_hsv));
    const auto cube_texture_diffuse_id = check(layer.storage.string.emplace("cube.texture.diffuse"_hsv));
    const auto cube_texture_specular_id = check(layer.storage.string.emplace("cube.texture.specular"_hsv));
    const auto cube_material_id = check(layer.storage.string.emplace("cube.material"_hsv));
    check(layer.storage.vertex.emplace(cube_vertex_id, [] {
        return game::create_vertex_component(std::span{ cube_vertices }, std::span{ cube_indices });
    }));
    const auto cube_texture_diffuse = check(layer.storage.texture.emplace(cube_texture_diffuse_id, [&root] {
        return game::create_texture_component(root / "textures/03_lightmap_diffuse.png");
    }));
    const auto cube_texture_specular = check(layer.storage.texture.emplace(cube_texture_specular_id, [&root] {
        return game::create_texture_component(root / "textures/03_lightmap_specular.png");
    }));
    check(layer.storage.material.emplace(cube_material_id, [&cube_texture_diffuse, &cube_texture_specular] {
        return game::material_component{
            .diffuse = cube_texture_diffuse,
            .specular = cube_texture_specular,
            .shininess = 128.0f * 0.6f,
        };
    }));

    const auto object_entities = //
        ranges::views::enumerate(cube_positions) | ranges::views::transform([&](auto ip) {
            auto [i, p] = ip;
            const float angle = 20.0f * (static_cast<float>(i));
            const entt::entity entity = layer.registry.create();
            layer.registry.emplace<game::shader_component<game::layer>::id>(entity, object_shader_id);
            layer.registry.emplace<game::vertex_component::id>(entity, cube_vertex_id);
            layer.registry.emplace<game::material_component::id>(entity, cube_material_id);
            layer.registry.emplace<game::transform_component>(
                entity,
                game::transform_component{ .tf{
                    .tr = p,
                    .rot = glm::angleAxis(glm::radians(angle), gfx_system.world.up()),
                } }
            );
            return entity;
        })
        | ranges::to<std::vector>();
    const sl::meta::defer destroy_object_entities{ [&] {
        layer.registry.destroy(object_entities.begin(), object_entities.end());
    } };

    const auto point_light_entities = //
        ranges::views::zip(point_light_positions, point_light_components) | ranges::views::transform([&](auto pl) {
            auto [p, l] = pl;
            const entt::entity entity = layer.registry.create();
            layer.registry.emplace<game::shader_component<game::layer>::id>(entity, source_shader_id);
            layer.registry.emplace<game::vertex_component::id>(entity, cube_vertex_id);
            layer.registry.emplace<game::point_light_component>(entity, l);
            layer.registry.emplace<game::transform_component>(
                entity, game::transform_component{ .tf{ .tr = p, .rot{} } }
            );
            return entity;
        })
        | ranges::to<std::vector>();
    const sl::meta::defer destroy_point_light_entities{ [&] {
        layer.registry.destroy(point_light_entities.begin(), point_light_entities.end());
    } };

    const entt::entity global_entity = [&] {
        const entt::entity entity = layer.registry.create();
        layer.registry.emplace<game::directional_light_component>(entity, global_directional_light_component);
        auto rot = glm::rotation(gfx_system.world.forward(), glm::normalize(glm::vec3{ -0.2f, -1.0f, -0.3f }));
        layer.registry.emplace<game::transform_component>(
            entity, game::transform_component{ .tf{ .tr{}, .rot{ rot } } }
        );
        layer.registry.emplace<game::input_component<game::layer>>(
            entity,
            [](const gfx::basis&,
               gfx::current_window& cw,
               game::layer&,
               entt::entity,
               game::input_state& input,
               const rt::time_point&) {
                input.keyboard.pressed.at(GLFW_KEY_ESCAPE).then([&cw](bool esc_pressed) {
                    cw.set_should_close(esc_pressed);
                });
            }
        );
        return entity;
    }();
    const sl::meta::defer destroy_global_entity{ [&] { layer.registry.destroy(global_entity); } };

    const entt::entity player_entity = [&] {
        const entt::entity entity = layer.registry.create();
        layer.registry.emplace<game::spot_light_component>(entity, player_spot_light_component);
        layer.registry.emplace<game::transform_component>(
            entity,
            gfx::transform{
                .tr = glm::vec3{ 0.0f, 0.0f, 3.0f },
                .rot = glm::angleAxis(glm::radians(-180.0f), gfx_system.world.up()),
            }
        );
        layer.registry.emplace<game::camera_component>(
            entity,
            gfx::perspective_projection{
                .fov = glm::radians(45.0f),
                .near = 0.1f,
                .far = 100.0f,
            }
        );
        layer.registry.emplace<game::input_component<game::layer>>(
            entity,
            [](const gfx::basis& world,
               gfx::current_window& cw,
               game::layer& layer,
               entt::entity entity,
               game::input_state& input,
               const rt::time_point& time_point) {
                constexpr auto calculate_tr = [](const gfx::basis& world, const game::keyboard_input_state& keyboard) {
                    constexpr auto sub = [](bool a, bool b) {
                        return static_cast<float>(static_cast<int>(a) - static_cast<int>(b));
                    };
                    const auto& q = keyboard.pressed.at(GLFW_KEY_Q);
                    const auto& w = keyboard.pressed.at(GLFW_KEY_W);
                    const auto& e = keyboard.pressed.at(GLFW_KEY_E);
                    const auto& a = keyboard.pressed.at(GLFW_KEY_A);
                    const auto& s = keyboard.pressed.at(GLFW_KEY_S);
                    const auto& d = keyboard.pressed.at(GLFW_KEY_D);
                    return sub(e.get(), q.get()) * world.up() + //
                           sub(d.get(), a.get()) * world.right() + //
                           sub(w.get(), s.get()) * world.forward();
                };

                if (const bool has_components = layer.registry.all_of<game::transform_component>(entity);
                    !ASSUME_VAL(has_components)) {
                    return;
                }

                auto& rmb_pressed = input.mouse.pressed.at(GLFW_MOUSE_BUTTON_RIGHT);
                rmb_pressed.then([&cw](bool cursor_hidden) {
                    cw.set_input_mode(GLFW_CURSOR, cursor_hidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                });

                auto& tf = layer.registry.get<game::transform_component>(entity).tf;
                if (rmb_pressed.get()) {
                    input.cursor.offset().map([&world, &tf](const glm::dvec2& cursor_offset) {
                        constexpr float sensitivity = -glm::radians(0.2f);
                        const float yaw = static_cast<float>(cursor_offset.x) * sensitivity;
                        const float pitch = static_cast<float>(cursor_offset.y) * sensitivity;

                        const glm::quat rot_yaw = glm::angleAxis(yaw, world.y);
                        tf.rotate(rot_yaw);

                        const glm::vec3 camera_forward = tf.rot * world.forward();
                        const glm::vec3 camera_right = glm::cross(camera_forward, world.up());
                        const glm::quat rot_pitch = glm::angleAxis(pitch, camera_right);
                        tf.rotate(rot_pitch);
                    });
                }
                {
                    constexpr float acc = 5.0f;
                    const float speed = acc * time_point.delta_sec().count();
                    const auto tr = calculate_tr(world, input.keyboard);
                    tf.translate(speed * (tf.rot * tr));
                }
            }
        );
        return entity;
    }();
    const sl::meta::defer destroy_player_entity{ [&] { layer.registry.destroy(player_entity); } };

    rt::time time;

    while (!gfx.current_window.should_close()) {
        // input
        gfx.context->poll_events();
        gfx.state->frame_buffer_size.then([&cw = gfx.current_window](glm::ivec2 frame_buffer_size) {
            cw.viewport(glm::ivec2{}, frame_buffer_size);
        });
        gfx.state->window_content_scale.then([](glm::fvec2 window_content_scale) {
            ImGui::GetStyle().ScaleAllSizes(window_content_scale.x);
        });

        // update
        const rt::time_point time_point = time.calculate();
        input_system(gfx_system.world, gfx.current_window, layer, time_point);

        // render
        const auto gfx_frame = gfx.new_frame();
        gfx_system(gfx_frame, *gfx.state, layer);

        // overlay
        auto imgui_frame = gfx.imgui.new_frame(); // TODO: require gfx_frame here too
        if (const auto imgui_window = imgui_frame.begin("debug")) {
            // TODO: maybe use actual angles against the world basis
            auto& directional_light_rot = layer.registry.get<game::transform_component>(global_entity).tf.rot;
            ImGui::SliderFloat4("directional_light rot", glm::value_ptr(directional_light_rot), -1.0f, 1.0f);
            directional_light_rot = glm::normalize(directional_light_rot);

            auto& directional_light = layer.registry.get<game::directional_light_component>(global_entity);
            ImGui::ColorEdit3("directional_light ambient", glm::value_ptr(directional_light.ambient));
            ImGui::ColorEdit3("directional_light diffuse", glm::value_ptr(directional_light.diffuse));
            ImGui::ColorEdit3("directional_light specular", glm::value_ptr(directional_light.specular));

            ImGui::Spacing();
            layer.storage.material.lookup(cube_material_id)
                .map([](sl::meta::persistent<game::material_component> material) {
                    ImGui::SliderFloat(
                        "shininess", &material->shininess, 2.0f, 256, "%.0f", ImGuiSliderFlags_Logarithmic
                    );
                });
        }
    }

    return 0;
}
