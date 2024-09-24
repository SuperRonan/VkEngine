#version 460

#define BIND_SINGLE_MATERIAL 0
#define BIND_SCENE 1
#define BIND_RENDERER_SET 1

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
	MaterialTextureIds textures;
	if(material_id != uint(-1))
	{
		textures = scene_pb_materials_textures.ids[material_id];
		material_props = scene_pb_materials[material_id].props;
	}
	else
	{
		material_props = NoMaterialProps();
		textures = NoMaterialTextureIds();
	}
	

	const vec2 uv = v_uv;
	const vec3 position = v_w_position; 
	const vec3 a_normal = normalize(v_w_normal);
	const vec3 normal = a_normal;
	vec3 tangent = safeNormalize(v_w_tangent);
	tangent = safeNormalize(tangent - dot(tangent, normal) * normal);
	const vec3 bi_tangent = cross(tangent, normal);

	GeometryShadingInfo geom;
	geom.position = position;
	geom.vertex_shading_normal = normal;

	geom.geometry_normal = normalize(cross(dFdy(geom.position), dFdx(geom.position)));

#if SHADING_FORCE_MAX_NORMAL_LEVEL >= SHADING_NORMAL_LEVEL_VERTEX
	geom.shading_normal = geom.vertex_shading_normal;
#else
	geom.shading_normal = geom.geometry_normal;
#endif

	geom.vertex_shading_tangent = tangent;

	PBMaterialSampleData material = readMaterial(material_id, uv);

	if(material.alpha <= 0.25f)
	{
		discard;
	}

#if SHADING_FORCE_MAX_NORMAL_LEVEL >= SHADING_NORMAL_LEVEL_TEXTURE
	if(material.normal.z != 0)
	{
		const mat3 TBN = mat3(geom.vertex_shading_tangent, bi_tangent, geom.vertex_shading_normal);
		geom.shading_normal = TBN * material.normal;
	}
#endif

	const vec3 camera_position = GetCameraWorldPosition(ubo.camera);
	const vec3 wo = normalize(camera_position - position);

	vec3 res = shade(geom, wo, material);
	
	res += material.albedo * scene_ubo.ambient;

	o_color = vec4(res, 1);
}