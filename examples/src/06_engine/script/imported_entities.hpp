//
// Created by usatiynyan.
//

#pragma once

#include "../common.hpp"

#include <bit>

namespace script {

inline exec::async<game::material> create_material(
    engine::layer& layer,
    sl::meta::unique_string texture_id,
    const std::filesystem::path& texture_path
) {
    const auto texture_result = co_await layer.loader.texture.emplace(texture_id, create_texture(texture_path, false));
    ASSERT(texture_result.has_value());
    auto texture = texture_result.value();

    co_return game::material{
        .diffuse = texture,
        .specular = texture,
        .shininess = 128.0f * 0.6f,
    };
}

inline exec::async<entt::entity> spawn_imported_entity(
    engine::context& e_ctx [[maybe_unused]],
    engine::layer& layer,
    const example_context& example_ctx,
    const std::filesystem::path& asset_relpath
) {
    const auto object_shader_id = "object.shader"_us(layer.storage.string);
    const auto default_material_id = "crate.material"_us(layer.storage.string);

    const auto asset_path = example_ctx.asset_path / asset_relpath;
    const auto asset_id = asset_relpath.generic_string();
    const auto asset_directory = asset_path.parent_path();
    const auto asset_document = [&] {
        using iterator = std::istream_iterator<char>;

        std::ifstream asset_file{ asset_path };
        const auto asset_json = nlohmann::json::parse(iterator{ asset_file }, iterator{});
        return engine::import::document_from_json(asset_json, asset_directory)
            .map_error([](const std::string& err) { PANIC(err); })
            .value();
    }();

    for (const auto& [material_i, material] : ranges::views::enumerate(asset_document.materials)) {
        const fx::gltf::Material::Texture& material_texture = material.pbrMetallicRoughness.baseColorTexture;
        if (material_texture.index < 0) {
            continue;
        }
        const fx::gltf::Texture& texture =
            asset_document.textures.at(static_cast<std::uint32_t>(material_texture.index));
        ASSERT(texture.source >= 0 && static_cast<std::uint32_t>(texture.source) < asset_document.images.size());
        const fx::gltf::Image& image = asset_document.images.at(static_cast<std::uint32_t>(texture.source));
        ASSERT(!image.uri.empty());
        ASSERT(!image.IsEmbeddedResource(), "not supported yet");
        const auto texture_path = asset_directory / image.uri;

        const auto material_id = "{}.material[{}]"_ufs(asset_id, material_i)(layer.storage.string);
        const auto texture_id = "{}.texture"_ufs(image.uri)(layer.storage.string);
        ASSERT(co_await layer.loader.material.emplace(material_id, create_material(layer, texture_id, texture_path)));
        game::log::debug("material_id={} texture_id={}", material_id, texture_id);
    }

    for (const auto& [mesh_i, mesh_] : ranges::views::enumerate(asset_document.meshes)) {
        const auto mesh_id = "{}.mesh[{}]"_ufs(asset_id, mesh_i)(layer.storage.string);
        std::ignore = co_await layer.loader.mesh.emplace(mesh_id, [&, &mesh = mesh_]() -> exec::async<game::mesh> {
            game::mesh mesh_asset;

            for (const auto& [primitive_i, primitive] : ranges::views::enumerate(mesh.primitives)) {
                const auto primitive_id = "{}.primitive[{}]"_ufs(mesh_id, primitive_i)(layer.storage.string);
                const auto primitive_vertex_id = "{}.vertex"_ufs(primitive_id)(layer.storage.string);
                const auto primitive_material_id = [&, primitive_material_i = primitive.material] {
                    if (primitive_material_i == -1) {
                        return default_material_id;
                    }
                    // TODO: FIX THIS
                    auto material_id = "{}.material[{}]"_ufs(asset_id, primitive_material_i)(layer.storage.string);
                    if (!layer.storage.material.lookup(material_id).has_value()) {
                        return default_material_id;
                    }
                    return material_id;
                }();

                const std::uint32_t vert_attr = primitive.attributes.at("POSITION");
                const fx::gltf::Accessor& vert_accessor = asset_document.accessors.at(vert_attr);
                ASSERT(vert_accessor.componentType == fx::gltf::Accessor::ComponentType::Float);
                ASSERT(vert_accessor.type == fx::gltf::Accessor::Type::Vec3);
                ASSERT(vert_accessor.count > 0);
                const fx::gltf::BufferView& vert_buffer_view =
                    asset_document.bufferViews.at(static_cast<std::uint32_t>(vert_accessor.bufferView));
                const fx::gltf::Buffer& vert_buffer =
                    asset_document.buffers.at(static_cast<std::uint32_t>(vert_buffer_view.buffer));
                const auto* vert_data_begin =
                    vert_buffer.data.data() + vert_buffer_view.byteOffset + vert_accessor.byteOffset;
                const auto* vert_data_end = vert_data_begin + vert_buffer_view.byteLength;
                const auto vert_at = [vert_data_begin,
                                      vert_data_end,
                                      byte_stride = vert_buffer_view.byteStride,
                                      count = vert_accessor.count](std::uint32_t i) {
                    using element_type = gfx::va_attrib_field<3, float>;
                    ASSERT(i < count);
                    const auto* vert_data_addr =
                        vert_data_begin + (byte_stride == 0 ? sizeof(element_type) : byte_stride) * i;
                    ASSERT(vert_data_addr < vert_data_end);
                    return *std::bit_cast<const element_type*>(vert_data_addr);
                };

                const std::uint32_t normal_attr = primitive.attributes.at("NORMAL");
                const fx::gltf::Accessor& normal_accessor = asset_document.accessors.at(normal_attr);
                ASSERT(normal_accessor.componentType == fx::gltf::Accessor::ComponentType::Float);
                ASSERT(normal_accessor.type == fx::gltf::Accessor::Type::Vec3);
                ASSERT(normal_accessor.count > 0);
                const fx::gltf::BufferView& normal_buffer_view =
                    asset_document.bufferViews.at(static_cast<std::uint32_t>(normal_accessor.bufferView));
                const fx::gltf::Buffer& normal_buffer =
                    asset_document.buffers.at(static_cast<std::uint32_t>(normal_buffer_view.buffer));
                const auto* normal_data_begin =
                    normal_buffer.data.data() + normal_buffer_view.byteOffset + normal_accessor.byteOffset;
                const auto* normal_data_end = normal_data_begin + normal_buffer_view.byteLength;
                const auto normal_at = [normal_data_begin,
                                        normal_data_end,
                                        byte_stride = normal_buffer_view.byteStride,
                                        count = normal_accessor.count](std::uint32_t i) {
                    using element_type = gfx::va_attrib_field<3, float>;
                    ASSERT(i < count);
                    const auto* normal_data_addr =
                        normal_data_begin + (byte_stride == 0 ? sizeof(element_type) : byte_stride) * i;
                    ASSERT(normal_data_addr < normal_data_end);
                    return *std::bit_cast<const element_type*>(normal_data_addr);
                };

                const std::uint32_t tex_coord_attr = primitive.attributes.at("TEXCOORD_0");
                const fx::gltf::Accessor& tex_coord_accessor = asset_document.accessors.at(tex_coord_attr);
                ASSERT(tex_coord_accessor.componentType == fx::gltf::Accessor::ComponentType::Float);
                ASSERT(tex_coord_accessor.type == fx::gltf::Accessor::Type::Vec2);
                ASSERT(tex_coord_accessor.count > 0);
                const fx::gltf::BufferView& tex_coord_buffer_view =
                    asset_document.bufferViews.at(static_cast<std::uint32_t>(tex_coord_accessor.bufferView));
                const fx::gltf::Buffer& tex_coord_buffer =
                    asset_document.buffers.at(static_cast<std::uint32_t>(tex_coord_buffer_view.buffer));
                const auto* tex_coord_data_begin =
                    tex_coord_buffer.data.data() + tex_coord_buffer_view.byteOffset + tex_coord_accessor.byteOffset;
                const auto* tex_coord_data_end = tex_coord_data_begin + tex_coord_buffer_view.byteLength;
                const auto tex_coord_at = [tex_coord_data_begin,
                                           tex_coord_data_end,
                                           byte_stride = tex_coord_buffer_view.byteStride,
                                           count = tex_coord_accessor.count](std::uint32_t i) {
                    using element_type = gfx::va_attrib_field<2, float>;
                    ASSERT(i < count);
                    const auto* tex_coord_data_addr =
                        tex_coord_data_begin + (byte_stride == 0 ? sizeof(element_type) : byte_stride) * i;
                    ASSERT(tex_coord_data_addr < tex_coord_data_end);
                    return *std::bit_cast<const element_type*>(tex_coord_data_addr);
                };

                const std::vector<VNT> vnts = [&] {
                    const std::uint32_t vnt_count =
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

                const fx::gltf::Accessor& indices_accessor =
                    asset_document.accessors.at(static_cast<std::uint32_t>(primitive.indices));
                ASSERT(
                    indices_accessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort
                    || indices_accessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedInt
                );
                ASSERT(indices_accessor.type == fx::gltf::Accessor::Type::Scalar);
                const fx::gltf::BufferView& indices_buffer_view =
                    asset_document.bufferViews.at(static_cast<std::uint32_t>(indices_accessor.bufferView));
                ASSERT(indices_buffer_view.byteStride == 0);
                const fx::gltf::Buffer& indices_buffer =
                    asset_document.buffers.at(static_cast<std::uint32_t>(indices_buffer_view.buffer));

                const auto f = [&](const auto* indices_data) -> exec::async<void> {
                    const std::span indices_span{ indices_data, indices_accessor.count };
                    ASSERT(co_await layer.loader.vertex.emplace(
                        primitive_vertex_id, create_vertex(std::span(vnts), indices_span)
                    ));
                };

                const auto* indices_data =
                    indices_buffer.data.data() + indices_buffer_view.byteOffset + indices_accessor.byteOffset;
                if (indices_accessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort) {
                    co_await f(std::bit_cast<const std::uint16_t*>(indices_data));
                } else {
                    co_await f(std::bit_cast<const std::uint32_t*>(indices_data));
                }

                ASSERT(co_await layer.loader.primitive.emplace(
                    primitive_id,
                    [primitive_vertex_id, primitive_material_id]() -> exec::async<game::primitive> {
                        co_return game::primitive{
                            .vtx{ primitive_vertex_id },
                            .mtl{ primitive_material_id },
                        };
                    }()
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
        node_entities.reserve(asset_document.nodes.size());
        for (std::size_t i = 0; i < asset_document.nodes.size(); ++i) {
            const entt::entity node_entity = layer.registry.create();
            node_entities.push_back(node_entity);
            game::log::debug("node_entity={}", static_cast<entt::id_type>(node_entity));
        }
        return node_entities;
    }();

    for (const auto& [node_entity, node_document] : ranges::views::zip(node_entities, asset_document.nodes)) {
        layer.registry.emplace<game::local_transform>(node_entity, [&node = node_document] {
            if (node.matrix != fx::gltf::defaults::IdentityMatrix) {
                return *ASSERT_VAL(game::transform::from_mat4(std::bit_cast<glm::mat4>(node.matrix)));
            }

            return game::transform{
                .tr = std::bit_cast<glm::vec3>(node.translation),
                .rot = std::bit_cast<glm::quat>(node.rotation),
                .s = std::bit_cast<glm::vec3>(node.scale),
            };
        }());

        for (const std::int32_t child_i : node_document.children) {
            ASSERT(child_i >= 0 && static_cast<std::uint32_t>(child_i) < node_entities.size());
            const entt::entity child_entity = node_entities.at(static_cast<std::uint32_t>(child_i));
            game::node::attach_child(layer, node_entity, child_entity);
        }

        if (node_document.mesh >= 0) {
            const auto mesh_id = "{}.mesh[{}]"_ufs(asset_id, node_document.mesh)(layer.storage.string);
            const auto mesh_asset = co_await layer.loader.mesh.get(mesh_id);
            for (const auto& primitive : mesh_asset->primitives) {
                const auto primitive_asset = co_await layer.loader.primitive.get(primitive.id);
                const entt::entity primitive_entity = layer.registry.create();
                layer.registry.emplace<game::shader<engine::layer>::id>(primitive_entity, object_shader_id);
                layer.registry.emplace<game::vertex::id>(primitive_entity, primitive_asset->vtx);
                layer.registry.emplace<game::material::id>(primitive_entity, primitive_asset->mtl);
                layer.registry.emplace<game::local_transform>(primitive_entity, game::transform{});
                game::node::attach_child(layer, node_entity, primitive_entity);
            }
            game::log::debug("node_entity={} attached mesh={}", static_cast<entt::id_type>(node_entity), mesh_id);
        }
    }

    ASSERT(asset_document.scene >= 0);
    const fx::gltf::Scene& asset_scene_document =
        asset_document.scenes.at(static_cast<std::uint32_t>(asset_document.scene));
    entt::entity imported_scene = layer.registry.create();
    layer.registry.emplace<game::local_transform>(imported_scene, game::transform{});
    for (const std::uint32_t node_i : asset_scene_document.nodes) {
        ASSERT(node_i < node_entities.size());
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

} // namespace script
