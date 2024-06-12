#version 460 core

layout(location = 0) in vec3 in_vert;

uniform mat4 u_transform;

void main() {
    gl_Position = u_transform * vec4(in_vert, 1.0);
}
