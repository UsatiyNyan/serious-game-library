//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/system/render.hpp"

#include <glm/glm.hpp>

namespace sl::game::engine::render {

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

namespace buffer {

struct directional_light_element {
    using component_type = directional_light;

    template <GfxLayerRequirements Layer>
    [[nodiscard]] static auto from(const Layer& layer, entt::entity entity, const component_type& component) {
        const auto& tf = *ASSERT_VAL(layer.registry.template try_get<transform>(entity));
        return directional_light_element{
            .direction = layer.world.direction(tf),
            .ambient = component.ambient,
            .diffuse = component.diffuse,
            .specular = component.specular,
        };
    }

public:
    alignas(16) glm::vec3 direction;

    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;
};

struct point_light_element {
    using component_type = point_light;

    template <GfxLayerRequirements Layer>
    [[nodiscard]] static auto from(const Layer& layer, entt::entity entity, const component_type& component) {
        const auto& tf = *ASSERT_VAL(layer.registry.template try_get<transform>(entity));
        return point_light_element{
            .position = tf.tr,
            .ambient = component.ambient,
            .diffuse = component.diffuse,
            .specular = component.specular,
            .constant = component.constant,
            .linear = component.linear,
            .quadratic = component.quadratic,
        };
    }

public:
    alignas(16) glm::vec3 position;

    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct spot_light_element {
    using component_type = spot_light;

    template <GfxLayerRequirements Layer>
    [[nodiscard]] static auto from(const Layer& layer, entt::entity entity, const component_type& component) {
        const auto& tf = *ASSERT_VAL(layer.registry.template try_get<transform>(entity));
        return buffer::spot_light_element{
            .position = tf.tr,
            .direction = layer.world.direction(tf),
            .ambient = component.ambient,
            .diffuse = component.diffuse,
            .specular = component.specular,
            .constant = component.constant,
            .linear = component.linear,
            .quadratic = component.quadratic,
            .cutoff = component.cutoff,
            .outer_cutoff = component.outer_cutoff,
        };
    }

public:
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
};

} // namespace buffer
} // namespace sl::game::engine::render
