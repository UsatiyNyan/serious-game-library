#version 460 core

struct light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform vec3 u_view_pos;
uniform light u_light;
uniform material u_material;

in vec3 msg_normal;
in vec3 msg_frag_pos;

out vec4 frag_color;

void main() {
    const vec3 ambient = u_light.ambient * u_material.ambient;

    const vec3 normal = normalize(msg_normal);
    const vec3 light_direction = normalize(u_light.position - msg_frag_pos);
    const float diffuse_angle = max(dot(normal, light_direction), 0.0f);
    const vec3 diffuse = u_light.diffuse * u_material.diffuse * diffuse_angle;

    const vec3 view_direction = normalize(u_view_pos - msg_frag_pos);
    const vec3 reflect_direction = reflect(-light_direction, normal);
    const float specular_angle = max(dot(view_direction, reflect_direction), 0.0f);
    const vec3 specular = u_light.specular * u_material.specular * pow(specular_angle, u_material.shininess);

    frag_color = vec4(ambient + diffuse + specular, 1.0);
}
