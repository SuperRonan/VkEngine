#version 460



#include <ShaderLib:/random.glsl>

#include "IndirectCommon.glsl"

layout(location = 0) in flat uvec4 v_flat;
layout(location = 1) in vec3 v_w_position;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec3 v_w_normal;

layout(location = 0) out vec4 o_albedo;
layout(location = 1) out vec4 o_position;
layout(location = 2) out vec4 o_normal; 

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
	const vec3 normal = normalize(v_w_normal);
	const vec3 albedo = readAlbedo(material_props, textures.albedo_texture_id, uv);

	o_albedo = vec4(albedo, 0);
	o_position = vec4(position, 0);
	o_normal = vec4(normal, 0);
}