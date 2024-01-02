#version 460 core

layout(location = 0) in vec3 world_position;
layout(location = 1) in vec3 world_normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 color;

void main()
{

    const vec3 light_dir = normalize(vec3(2, 0.2, 1));
    const vec3 light_color = vec3(1, 0.8, 0.6);
    const vec3 ambient = vec3(0.1);

    const vec3 albedo = vec3(0.2, 0.3, 1.0);
    const float alpha = 0.5;

    // color = vec4(albedo * max(0.0, dot(world_normal, light_dir)) * light_color + albedo * ambient, 1.0);
    color = vec4(albedo, alpha);
}