#version 460

#include "common.glsl"

#define BIND_SINGLE_MATERIAL 0
#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

layout(location = 0) in vec3 v_w_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_w_normal;

layout(location = 0) out vec4 o_albedo;
layout(location = 1) out vec4 o_position;
layout(location = 2) out vec4 o_normal; 

void main()
{
	// const PBMaterialProperties props = material_props.props;
	const vec2 uv = v_uv;
	const vec3 position = v_w_position; 
	const vec3 normal = normalize(v_w_normal);
	const vec3 albedo = vec3(1, 1, 1);

	o_albedo = vec4(albedo, 0);
	o_position = vec4(position, 0);
	o_normal = vec4(normal, 0);
}