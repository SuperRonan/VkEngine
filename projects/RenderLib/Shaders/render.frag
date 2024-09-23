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
	PBMaterialSampleData material = readBoundMaterial(uv);
	GeometryShadingInfo geom;
	geom.position = v_w_position;
	const vec3 a_normal = normalize(v_w_normal);
	geom.vertex_shading_normal = a_normal;

	geom.geometry_normal = geom.vertex_shading_normal;
	geom.geometry_normal = normalize(cross(dFdy(geom.position), dFdx(geom.position)));

#if SHADING_FORCE_MAX_NORMAL_LEVEL >= SHADING_NORMAL_LEVEL_VERTEX
	geom.shading_normal = geom.vertex_shading_normal;
#else
	geom.shading_normal = geom.geometry_normal;
#endif

	geom.vertex_shading_tangent = safeNormalize(v_w_tangent);
	geom.vertex_shading_tangent = safeNormalize(geom.vertex_shading_tangent - dot(geom.vertex_shading_tangent, geom.vertex_shading_normal) * geom.vertex_shading_normal);
	const vec3 bi_tangent = cross(geom.vertex_shading_tangent, geom.vertex_shading_normal);
	
#if SHADING_FORCE_MAX_NORMAL_LEVEL >= SHADING_NORMAL_LEVEL_TEXTURE
	if(material.normal.z != 0)
	{
		const mat3 TBN = mat3(geom.vertex_shading_tangent, bi_tangent, geom.vertex_shading_normal);
		geom.shading_normal = TBN * material.normal;
	}
#endif

	const vec3 camera_position = GetCameraWorldPosition(ubo.camera);
	const vec3 wo = normalize(camera_position - geom.position);

	vec3 res = shade(geom, wo, material);
	res += material.albedo * scene_ubo.ambient;

	const float ds = dot(geom.geometry_normal, geom.shading_normal);
	const float dvs = dot(geom.geometry_normal, geom.vertex_shading_normal);
	res *= ds;

	// if( > 0)
	// {
	// 	res *= vec3(0, 1, 0);
	// }
	// else
	// {
	// 	res *= vec3(1, 0, 0);
	// }


	o_color = vec4(res, 1);
}