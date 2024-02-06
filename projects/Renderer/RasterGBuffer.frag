#version 460

#include "common.glsl"

#define BIND_SINGLE_MATERIAL 1
#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

layout(location = 0) in vec3 v_w_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_w_normal;
layout(location = 3) in vec3 v_w_tangent;

layout(location = 0) out vec4 o_albedo;
layout(location = 1) out vec4 o_position;
layout(location = 2) out vec4 o_normal; 

void main()
{
	const PBMaterialProperties props = material_props.props;
	const vec2 uv = v_uv;
	const vec3 position = v_w_position; 
	const vec3 albedo = getBoundMaterialAlbedo(v_uv);
	const vec3 a_normal = normalize(v_w_normal);
	vec3 normal = a_normal;
	vec3 tangent = safeNormalize(v_w_tangent);
	tangent = safeNormalize(tangent - dot(tangent, normal) * normal);
	const vec3 bi_tangent = cross(normal, tangent);

	if((material_props.props.flags & MATERIAL_FLAG_USE_NORMAL_TEXTURE_BIT) != 0)
	{
		const mat3 TBN = mat3(tangent, bi_tangent, normal);
		vec3 tex_normal = texture(NormalTexture, uv).xyz;
		tex_normal = normalize(tex_normal * 2.0 - 1);
		normal = TBN * tex_normal;
	}

	o_albedo = vec4(albedo, 0);
	o_position = vec4(position, 0);
	o_normal = vec4(normal, 0);
}