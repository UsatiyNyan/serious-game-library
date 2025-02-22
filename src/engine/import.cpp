//
// Created by usatiynyan.
// TODO: fork and optimize
//

#include "sl/game/engine/import.hpp"

#include <libassert/assert.hpp>

namespace sl::game::engine::import {

meta::result<fx::gltf::Document, std::string> document_from_json(
    const nlohmann::json& document_json,
    std::filesystem::path document_root_path,
    fx::gltf::ReadQuotas read_quotas
) {
    const fx::gltf::detail::DataContext data_context{
        .bufferRootPath = std::move(document_root_path),
        .readQuotas = std::move(read_quotas),
        .binaryData = nullptr,
    };

    try {
        return fx::gltf::detail::Create(document_json, data_context);
    } catch (std::exception& ex) {
        return meta::err(std::string{ ex.what() });
    } catch (...) {
        PANIC("unknown error");
    }
}

struct load_binary_impl {
    using iterator_type = std::span<const std::byte>::iterator;

    template <typename T>
    meta::result<std::pair<iterator_type, const T*>, std::string> read_sized(iterator_type it, std::size_t size) const {
        if (std::distance(it, end) > size) {
            return meta::err(std::string{ "buffer overrun" });
        }
        return std::make_pair(std::next(it, size), std::bit_cast<const T*>(it.base()));
    }

public:
    iterator_type end;
};


meta::result<fx::gltf::Document, std::string> document_from_binary(
    std::span<const std::byte> document_bytes,
    std::filesystem::path document_root_path,
    fx::gltf::ReadQuotas read_quotas
) {
    const load_binary_impl impl{
        .end = document_bytes.end(),
    };

    auto header_it = document_bytes.begin();
    auto header_result = impl.read_sized<fx::gltf::detail::GLBHeader>(header_it, fx::gltf::detail::HeaderSize);
    if (!header_result.has_value()) {
        return meta::err(std::move(header_result.error()));
    }
    const auto [json_it, header_ptr] = header_result.value();

    const auto& header = *header_ptr;
    if (header.magic != fx::gltf::detail::GLBHeaderMagic
        || header.jsonHeader.chunkType != fx::gltf::detail::GLBChunkJSON
        || header.jsonHeader.chunkLength + fx::gltf::detail::HeaderSize > header.length) {
        return meta::err(std::string{ "invalid json glb header" });
    }

    const std::size_t header_json_size = fx::gltf::detail::HeaderSize + header.jsonHeader.chunkLength;
    if (header_json_size > read_quotas.MaxFileSize) {
        return meta::err(std::string{ "quota exceeded : json file size > MaxFileSize" });
    }

    auto json_result = impl.read_sized<std::uint8_t>(json_it, header.jsonHeader.chunkLength);
    if (!json_result.has_value()) {
        return meta::err(std::move(json_result.error()));
    }
    const auto [bin_header_it, json_ptr] = json_result.value();
    const auto json = std::span(json_ptr, header.jsonHeader.chunkLength);

    auto bin_header_result =
        impl.read_sized<fx::gltf::detail::ChunkHeader>(bin_header_it, fx::gltf::detail::ChunkHeaderSize);
    if (!bin_header_result.has_value()) {
        return meta::err(std::move(bin_header_result.error()));
    }
    const auto [bin_it, bin_header_ptr] = bin_header_result.value();
    const auto& bin_header = *bin_header_ptr;
    if (bin_header.chunkType != fx::gltf::detail::GLBChunkBIN) {
        return meta::err(std::string{ "invalid bin glb header" });
    }

    const std::size_t total_size = header_json_size + fx::gltf::detail::ChunkHeaderSize + bin_header.chunkLength;
    if (total_size > read_quotas.MaxFileSize) {
        return meta::err(std::string{ "quota exceeded : total file size > MaxFileSize" });
    }

    auto bin_result = impl.read_sized<std::uint8_t>(bin_it, bin_header.chunkLength);
    if (!bin_result.has_value()) {
        return meta::err(std::move(bin_result.error()));
    }
    const auto [end_it, bin_ptr] = bin_result.value();

    // TODO: this can be optimised
    std::vector<std::uint8_t> binary{ bin_ptr, bin_ptr + bin_header.chunkLength };

    const fx::gltf::detail::DataContext data_context{
        .bufferRootPath = std::move(document_root_path),
        .readQuotas = std::move(read_quotas),
        .binaryData = &binary,
    };

    try {
        return fx::gltf::detail::Create(nlohmann::json::parse(json.begin(), json.end()), data_context);
    } catch (std::exception& ex) {
        return meta::err(std::string{ ex.what() });
    } catch (...) {
        PANIC("unknown error");
    }
}

} // namespace sl::game::engine::import
