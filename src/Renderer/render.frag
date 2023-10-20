#version 460

#include "common.glsl"

#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_w_normal;

layout(location = 0) out vec4 o_color;

vec3 shade()
{
	vec3 res = 0..xxx;
	const PBMaterialProperties props = material_props.props;
	const vec3 albedo = getBoundMaterialAlbedo(v_uv);

	res += albedo;

	return res;
}

void main()
{


	o_color = vec4(shade(), 1);
}