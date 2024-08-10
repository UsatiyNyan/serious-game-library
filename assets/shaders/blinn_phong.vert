#version 460 core

uniform mat4 u_model; // part of u_transform
uniform mat3 u_it_model; // for normals
uniform mat4 u_transform;

layout(location = 0) in vec3 in_vert;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coords;

out vec3 msg_frag_pos;
out vec3 msg_normal;
out vec2 msg_tex_coords;

void main() {
    gl_Position = u_transform * vec4(in_vert, 1.0);
    msg_frag_pos = vec3(u_model * vec4(in_vert, 1.0));
    msg_normal = u_it_model * in_normal;
    msg_tex_coords = in_tex_coords;
}

