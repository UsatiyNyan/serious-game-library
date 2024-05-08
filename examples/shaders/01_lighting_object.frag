#version 460 core

uniform vec3 u_light_color;
uniform vec3 u_light_pos;
uniform vec3 u_view_pos;

uniform vec3 u_object_color;
uniform float u_ambient_strength = 0.1;
uniform float u_specular_strength = 0.5;
uniform int u_shininess = 32;

in vec3 msg_normal;
in vec3 msg_frag_pos;

out vec4 frag_color;

void main() {
    const vec3 ambient_color = u_ambient_strength * u_light_color;

    const vec3 normal = normalize(msg_normal); // just to make sure
    const vec3 light_direction = normalize(u_light_pos - msg_frag_pos);
    const float diffuse_angle = max(dot(normal, light_direction), 0.0f);
    const vec3 diffuse_color = diffuse_angle * u_light_color;

    const vec3 view_direction = normalize(u_view_pos - msg_frag_pos);
    const vec3 reflect_direction = reflect(-light_direction, normal);
    const float specular_angle = max(dot(view_direction, reflect_direction), 0.0f);
    const vec3 specular_color = u_specular_strength * pow(specular_angle, u_shininess) * u_light_color;

    const vec3 resulting_color = (ambient_color + diffuse_color + specular_color) * u_object_color;
    frag_color = vec4(resulting_color, 1.0);
}
