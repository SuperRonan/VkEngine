#version 460

#define BIND_SINGLE_MATERIAL 1

#include "common.glsl"
#include "shading.glsl"

#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

#define BIND_SCENE 1
#define LIGHTS_ACCESS readonly
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

layout(location = 0) in vec3 v_w_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_w_normal;
layout(location = 3) in vec3 v_w_tangent;

layout(location = 0) out vec4 o_color;

void main()
{
	const PBMaterialProperties props = material_props.props;
	const vec2 uv = v_uv;
	const vec3 position = v_w_position; 
	vec3 albedo = getBoundMaterialAlbedo(v_uv);
	
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

	vec3 res = shade(albedo, position, normal);
	res += albedo * scene_ubo.ambient;

	o_color = vec4(res, 1);
}