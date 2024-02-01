#version 460

#define BIND_SINGLE_MATERIAL 0

#include "common.glsl"
#include "shading.glsl"

#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

#define BIND_SCENE 1
#define LIGHTS_ACCESS readonly
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

layout(location = 0) in vec3 v_w_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_w_normal;

layout(location = 0) out vec4 o_color;

void main()
{
	// const PBMaterialProperties props = material_props.props;
	const vec2 uv = v_uv;
	const vec3 position = v_w_position; 
	const vec3 normal = normalize(v_w_normal);
	const vec3 albedo = vec3(1, 1, 1);//getBoundMaterialAlbedo(v_uv);

	vec3 res = shade(albedo, position, normal);
	res += albedo * scene_ubo.ambient;

	o_color = vec4(res, 1);
}