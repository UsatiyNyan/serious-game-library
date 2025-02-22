//
// Created by usatiynyan.
//

#pragma once

#include <fx/gltf.h>

#include <sl/meta/monad/result.hpp>

#include <cstddef>
#include <span>

namespace sl::game::engine::import {

meta::result<fx::gltf::Document, std::string> document_from_json(
    const nlohmann::json& document_json,
    std::filesystem::path document_root_path = {},
    fx::gltf::ReadQuotas read_quotas = {}
);

meta::result<fx::gltf::Document, std::string> document_from_binary(
    std::span<const std::byte> document_bytes,
    std::filesystem::path document_root_path = {},
    fx::gltf::ReadQuotas read_quotas = {}
);

} // namespace sl::game::engine::import
