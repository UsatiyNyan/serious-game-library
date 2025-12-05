//
// Created by usatiynyan.
// TODO:
// - use scale
// - skew and perspective in decompose
// - transform component should be a variant<{tr, rot, s, ...}, mat4>
//

#pragma once

#include <sl/meta/conn/dirty.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <sl/meta/monad/maybe.hpp>

namespace sl::game {

struct transform {
    glm::vec3 tr;
    glm::quat rot = glm::identity<glm::quat>();
    glm::vec3 s{ 1.0f, 1.0f, 1.0f }; // use this component

    static meta::maybe<transform> from_mat4(const glm::mat4& mat4) {
        transform result;
        // TODO: skew and perspective
        glm::vec3 skew;
        glm::vec4 perspective;
        if (!glm::decompose(mat4, result.s, result.rot, result.tr, skew, perspective)) {
            return meta::null;
        }
        return result;
    }

    void translate(const glm::vec3& a_tr) & { tr += a_tr; }
    void rotate(const glm::quat& a_rot) & { rot = glm::normalize(a_rot * rot); }
    void scale(const glm::vec3& a_s) & { s = a_s; }

    void normalize() & {
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
inline transform scale(transform tf, const glm::vec3& tr) {
    tf.scale(tr);
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
        .s = b.s * a.s,
    };
}

// for node updates use this one
// local_transform_system will check if local_tranform is changed and apply transforms down the tree
using local_transform = meta::dirty<transform>;

} // namespace sl::game
