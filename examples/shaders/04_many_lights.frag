#version 460 core

struct material_data {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct material {
    vec3 diffuse;
    vec3 specular;
};

struct directional_light {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct point_light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct spot_light {
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;

    float cutoff;
    float outer_cutoff;
};

uniform vec3 u_view_pos;

uniform material_data u_material;

uniform directional_light u_directional_light;

uniform uint u_point_light_size = 0;
layout(std430, binding = 0) readonly buffer b_point_lights {
    point_light b_point_lights_data[];
};

uniform uint u_spot_light_size = 0;
layout(std430, binding = 1) readonly buffer b_spot_lights {
    spot_light b_spot_lights_data[];
};

in vec3 msg_frag_pos;
in vec3 msg_normal;
in vec2 msg_tex_coords;

out vec4 frag_color;

vec3 calc_directional_light(directional_light light, material material, vec3 normal, vec3 view_direction) {
    const vec3 light_direction = normalize(-light.direction);

    const vec3 ambient = light.ambient * material.diffuse;

    const float diffuse_angle = max(dot(normal, light_direction), 0.0f);
    const vec3 diffuse = light.diffuse * material.diffuse * diffuse_angle;

    const vec3 reflect_direction = reflect(-light_direction, normal);
    const float specular_angle = max(dot(view_direction, reflect_direction), 0.0f);
    const vec3 specular = light.specular * material.specular * pow(specular_angle, u_material.shininess);

    return ambient + diffuse + specular;
}

vec3 calc_point_light(point_light light, material material, vec3 normal, vec3 frag_pos, vec3 view_direction) {
    const vec3 light_direction = normalize(light.position - frag_pos);

    const vec3 ambient = light.ambient * material.diffuse;

    const float diffuse_angle = max(dot(normal, light_direction), 0.0f);
    const vec3 diffuse = light.diffuse * material.diffuse * diffuse_angle;

    const vec3 reflect_direction = reflect(-light_direction, normal);
    const float specular_angle = max(dot(view_direction, reflect_direction), 0.0f);
    const vec3 specular = light.specular * material.specular * pow(specular_angle, u_material.shininess);

    const float distance = length(light.position - frag_pos);
    const float attenuation = 1.0f / (
            light.constant +
                light.linear * distance +
                light.quadratic * (distance * distance)
            );

    return (ambient + diffuse + specular) * attenuation;
}

vec3 calc_spot_light(spot_light light, material material, vec3 normal, vec3 frag_pos, vec3 view_direction) {
    const vec3 light_direction = normalize(light.position - frag_pos);

    const vec3 ambient = light.ambient * material.diffuse;

    const float diffuse_angle = max(dot(normal, light_direction), 0.0f);
    const vec3 diffuse = light.diffuse * material.diffuse * diffuse_angle;

    const vec3 reflect_direction = reflect(-light_direction, normal);
    const float specular_angle = max(dot(view_direction, reflect_direction), 0.0f);
    const vec3 specular = light.specular * material.specular * pow(specular_angle, u_material.shininess);

    const float distance = length(light.position - frag_pos);
    const float attenuation = 1.0f / (
            light.constant +
                light.linear * distance +
                light.quadratic * (distance * distance)
            );

    const float theta = dot(light_direction, normalize(-light.direction));
    const float epsilon = light.cutoff - light.outer_cutoff;
    const float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

    return (ambient + diffuse + specular) * attenuation * intensity;
}

void main() {
    material material;
    material.diffuse = texture(u_material.diffuse, msg_tex_coords).rgb;
    material.specular = texture(u_material.specular, msg_tex_coords).rgb;

    const vec3 normal = normalize(msg_normal);
    const vec3 view_direction = normalize(u_view_pos - msg_frag_pos);

    vec3 result = calc_directional_light(u_directional_light, material, normal, view_direction);
    for (uint i = 0; i < u_point_light_size; ++i) {
        result += calc_point_light(b_point_lights_data[i], material, normal, msg_frag_pos, view_direction);
    }
    for (uint i = 0; i < u_spot_light_size; ++i) {
        result += calc_spot_light(b_spot_lights_data[i], material, normal, msg_frag_pos, view_direction);
    }

    frag_color = vec4(result, 1.0);
}
