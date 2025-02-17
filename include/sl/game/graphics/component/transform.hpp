//
// Created by usatiynyan.
//

#pragma once

#include <sl/meta/conn/dirty.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace sl::game {

struct transform {
    glm::vec3 tr;
    glm::quat rot = glm::identity<glm::quat>();

    void translate(const glm::vec3& a_tr) { tr += a_tr; }
    void rotate(const glm::quat& a_rot) { rot = glm::normalize(a_rot * rot); }
    void normalize() {
        if (glm::length2(tr) > 0.0f) {
            tr = glm::normalize(tr);
        }
        rot = glm::normalize(rot);
    }
};

inline transform translate(transform tf, const glm::vec3& tr) {
    tf.translate(tr);
    return tf;
}
inline transform rotate(transform tf, const glm::quat& rot) {
    tf.rotate(rot);
    return tf;
}
inline transform normalize(transform tf) {
    tf.normalize();
    return tf;
}
inline transform combine(const transform& a, const transform& b) {
    return transform{
        .tr = b.rot * a.tr + b.tr,
        .rot = b.rot * a.rot,
    };
}

// for node updates use this one
// local_transform_system will check if local_tranform is changed and apply transforms down the tree
using local_transform = meta::dirty<transform>;

} // namespace sl::game
