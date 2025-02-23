//
// Created by usatiynyan.
//

#pragma once

#include "../common.hpp"

#include <bit>

namespace script {

inline exec::async<entt::entity> spawn_imported_entity(
    engine::context& e_ctx [[maybe_unused]],
    engine::layer& layer,
    const example_context& example_ctx,
    const std::filesystem::path& mesh_relpath
) {
    const entt::entity imported_entity = layer.registry.create();
    layer.registry.emplace<game::local_transform>(imported_entity, game::transform{});
    game::log::debug("imported_entity={}", static_cast<std::uint32_t>(imported_entity));

    const auto object_shader_id = "object.shader"_us(layer.storage.string);
    const auto crate_material_id = "crate.material"_us(layer.storage.string);

    const auto meshes_path = example_ctx.asset_path / "meshes";
    std::ifstream mesh_file{ meshes_path / mesh_relpath };
    using iterator = std::istream_iterator<char>;
    const auto mesh_json = nlohmann::json::parse(iterator{ mesh_file }, iterator{});
    const auto mesh_document = engine::import::document_from_json(mesh_json, meshes_path)
                                   .map_error([](const std::string& err) { PANIC(err); })
                                   .value();

    for (const fx::gltf::Mesh& mesh : mesh_document.meshes) {
        const entt::entity mesh_entity = layer.registry.create();
        layer.registry.emplace<game::local_transform>(mesh_entity, game::transform{});
        game::node::attach_child(layer, imported_entity, mesh_entity);
        game::log::debug("mesh_entity={}", static_cast<std::uint32_t>(mesh_entity));

        for (const auto& [primitive_i, primitive] : ranges::views::enumerate(mesh.primitives)) {
            const auto primitive_vertex_id_str = fmt::format("{}[{}].vertex", mesh.name, primitive_i);
            const auto primitive_vertex_id =
                layer.storage.string.insert(sl::meta::hash_string_view{ primitive_vertex_id_str });

            const std::uint32_t vert_attr = primitive.attributes.at("POSITION");
            const fx::gltf::Accessor& vert_accessor = mesh_document.accessors.at(vert_attr);
            ASSERT(vert_accessor.componentType == fx::gltf::Accessor::ComponentType::Float);
            ASSERT(vert_accessor.type == fx::gltf::Accessor::Type::Vec3);
            ASSERT(vert_accessor.count > 0);
            const fx::gltf::BufferView& vert_buffer_view =
                mesh_document.bufferViews.at(static_cast<std::uint32_t>(vert_accessor.bufferView));
            const fx::gltf::Buffer& vert_buffer =
                mesh_document.buffers.at(static_cast<std::uint32_t>(vert_buffer_view.buffer));
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
            const fx::gltf::Accessor& normal_accessor = mesh_document.accessors.at(normal_attr);
            ASSERT(normal_accessor.componentType == fx::gltf::Accessor::ComponentType::Float);
            ASSERT(normal_accessor.type == fx::gltf::Accessor::Type::Vec3);
            ASSERT(normal_accessor.count > 0);
            const fx::gltf::BufferView& normal_buffer_view =
                mesh_document.bufferViews.at(static_cast<std::uint32_t>(normal_accessor.bufferView));
            const fx::gltf::Buffer& normal_buffer =
                mesh_document.buffers.at(static_cast<std::uint32_t>(normal_buffer_view.buffer));
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
            const fx::gltf::Accessor& tex_coord_accessor = mesh_document.accessors.at(tex_coord_attr);
            ASSERT(tex_coord_accessor.componentType == fx::gltf::Accessor::ComponentType::Float);
            ASSERT(tex_coord_accessor.type == fx::gltf::Accessor::Type::Vec2);
            ASSERT(tex_coord_accessor.count > 0);
            const fx::gltf::BufferView& tex_coord_buffer_view =
                mesh_document.bufferViews.at(static_cast<std::uint32_t>(tex_coord_accessor.bufferView));
            const fx::gltf::Buffer& tex_coord_buffer =
                mesh_document.buffers.at(static_cast<std::uint32_t>(tex_coord_buffer_view.buffer));
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
            // game::log::debug(
            //     "vertices=[\n{}\n]",
            //     fmt::join(
            //         vnts | ranges::views::transform([](const VNT& vnt) {
            //             return fmt::format("({})", fmt::join(vnt.vert.value, ","));
            //         }),
            //         "\n"
            //     )
            // );

            const fx::gltf::Accessor& indices_accessor =
                mesh_document.accessors.at(static_cast<std::uint32_t>(primitive.indices));
            ASSERT(
                indices_accessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort
                || indices_accessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedInt
            );
            ASSERT(indices_accessor.type == fx::gltf::Accessor::Type::Scalar);
            const fx::gltf::BufferView& indices_buffer_view =
                mesh_document.bufferViews.at(static_cast<std::uint32_t>(indices_accessor.bufferView));
            ASSERT(indices_buffer_view.byteStride == 0);
            const fx::gltf::Buffer& indices_buffer =
                mesh_document.buffers.at(static_cast<std::uint32_t>(indices_buffer_view.buffer));

            const auto f = [&](const auto* indices_data) -> exec::async<sl::meta::unit> {
                const std::span indices_span{ indices_data, indices_accessor.count };
                // game::log::debug("indices=[{}]", fmt::join(indices_span, ","));
                ASSERT(co_await layer.loader.vertex.emplace(
                    primitive_vertex_id, create_vertex(std::span(vnts), indices_span)
                ));
                co_return sl::meta::unit{};
            };

            const auto* indices_data =
                indices_buffer.data.data() + indices_buffer_view.byteOffset + indices_accessor.byteOffset;
            if (indices_accessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort) {
                std::ignore = co_await f(std::bit_cast<const std::uint16_t*>(indices_data));
            } else {
                std::ignore = co_await f(std::bit_cast<const std::uint32_t*>(indices_data));
            }

            const entt::entity primitive_entity = layer.registry.create();
            layer.registry.emplace<game::shader<engine::layer>::id>(primitive_entity, object_shader_id);
            layer.registry.emplace<game::vertex::id>(primitive_entity, primitive_vertex_id);
            layer.registry.emplace<game::material::id>(primitive_entity, crate_material_id);
            layer.registry.emplace<game::local_transform>(primitive_entity, game::transform{});
            game::node::attach_child(layer, mesh_entity, primitive_entity);

            game::log::debug(
                "primitive_entity={} vertex_id={}",
                static_cast<std::uint32_t>(primitive_entity),
                primitive_vertex_id.string_view()
            );
        }
    }

    co_return imported_entity;
}

} // namespace script
