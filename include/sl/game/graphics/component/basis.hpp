//
// Created by usatiynyan.
//

#pragma once

#include "sl/game/graphics/component/transform.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace sl::game {

struct basis {
    glm::vec3 x{ 1.0f, 0.0f, 0.0f };
    glm::vec3 y{ 0.0f, 1.0f, 0.0f };
    glm::vec3 z{ 0.0f, 0.0f, 1.0f };

    glm::vec3 forward() const { return z; }
    glm::vec3 right() const { return -x; }
    glm::vec3 up() const { return y; }

    [[nodiscard]] basis rotate(const glm::quat& rot) const {
        return basis{
            .x = rot * x,
            .y = rot * y,
            .z = rot * z,
        };
    }

    [[nodiscard]] glm::mat4 view(const transform& tf) const {
        const glm::vec3& pos = tf.tr;
        const glm::vec3 forward = tf.rot * this->forward();
        const glm::vec3 up = tf.rot * this->up();
        return glm::lookAt(pos, pos + forward, up);
    }

    [[nodiscard]] glm::vec3 direction(const transform& tf) const { return tf.rot * forward(); }
};

} // namespace sl::game
