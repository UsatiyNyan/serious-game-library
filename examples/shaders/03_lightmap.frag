#version 460 core

struct material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform material u_material;
uniform light u_light;
uniform vec3 u_view_pos;

in vec3 msg_frag_pos;
in vec3 msg_normal;
in vec2 msg_tex_coords;

out vec4 frag_color;

void main() {
    const vec3 material_diffuse = texture(u_material.diffuse, msg_tex_coords).rgb;
    const vec3 material_specular = texture(u_material.specular, msg_tex_coords).rgb;

    const vec3 ambient = u_light.ambient * material_diffuse;

    const vec3 normal = normalize(msg_normal);
    const vec3 light_direction = normalize(u_light.position - msg_frag_pos);
    const float diffuse_angle = max(dot(normal, light_direction), 0.0f);
    const vec3 diffuse = u_light.diffuse * material_diffuse * diffuse_angle;

    const vec3 view_direction = normalize(u_view_pos - msg_frag_pos);
    const vec3 reflect_direction = reflect(-light_direction, normal);
    const float specular_angle = max(dot(view_direction, reflect_direction), 0.0f);
    const vec3 specular = u_light.specular * material_specular * pow(specular_angle, u_material.shininess);

    frag_color = vec4(ambient + diffuse + specular, 1.0);
}

