#version 460

#define BIND_SINGLE_MATERIAL 0

#include "common.glsl"
#include "shading.glsl"

#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

#include "IndirectCommon.glsl"

#define BIND_SCENE 1
#define LIGHTS_ACCESS readonly
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

layout(location = 0) in flat uvec4 v_flat; 
layout(location = 1) in vec3 v_w_position;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec3 v_w_normal;
layout(location = 4) in vec3 v_w_tangent;

layout(location = 0) out vec4 o_color;


vec3 readAlbedo(const in PBMaterialProperties props, uint texture_id, vec2 uv)
{
	vec3 res;
	if(texture_id != uint(-1))
	{
		res = texture(SceneTextures2D[texture_id], uv).xyz;
	}
	else
	{
		res = props.albedo;
	}
	return res;
}

void main()
{
	const uint draw_id = v_flat.x;
	const uint model_id = model_indices.index[draw_id];
	const uint material_id = scene_objects_table.table[model_id].material_id;
	PBMaterialProperties material_props;
	ScenePBMaterialTextures textures;
	if(material_id != uint(-1))
	{
		textures = scene_pb_materials_textures.ids[material_id];
		material_props = scene_pb_materials[material_id].props;
	}
	else
	{
		material_props = NoMaterialProps();
		textures = NoPBMaterialTextures();
	}
	

	const vec2 uv = v_uv;
	const vec3 position = v_w_position; 
	const vec3 albedo = readAlbedo(material_props, textures.albedo_texture_id, v_uv);
	const vec3 a_normal = normalize(v_w_normal);
	vec3 normal = a_normal;
	vec3 tangent = safeNormalize(v_w_tangent);
	tangent = safeNormalize(tangent - dot(tangent, normal) * normal);
	const vec3 bi_tangent = cross(tangent, normal);
	
	if((material_props.flags & MATERIAL_FLAG_USE_NORMAL_TEXTURE_BIT) != 0 && textures.normal_texture_id != -1)
	{
		const mat3 TBN = mat3(tangent, bi_tangent, normal);
		vec3 tex_normal = vec3(0.5, 0.5, 1);
		tex_normal = texture(SceneTextures2D[textures.normal_texture_id], uv).xyz;
		tex_normal = normalize(tex_normal * 2.0 - 1);
		normal = TBN * tex_normal;
	}

	vec3 res = shade(albedo, position, normal);
	res += albedo * scene_ubo.ambient;

	o_color = vec4(res, 1);
}