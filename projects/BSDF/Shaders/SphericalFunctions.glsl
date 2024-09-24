#pragma once

#include "Bindings.glsl"

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/Maths/transforms.glsl>

#include <RenderLibShaders:/shading.glsl>

#define NORMAL vec3(0, 1, 0)
#define TANGENT vec3(1, 0, 0)

float EvaluateSpecularBSDF(vec3 normal, vec3 wo, vec3 wi, float shininess)
{
	const float cos_theta_i = max(dot(normal, wi), 0);
	if(cos_theta_i == 0)
	{
		return 0.0f;
	}
	const vec3 refl = reflect(-wo, normal);
	const float cos_r = max(dot(refl, wi), 0);

	const float norm = (shininess + 1) / TWO_PI;

	return pow(cos_r, shininess) * norm;
}

float EvaluateSphericalFunction(uint index, vec3 wo, vec3 wi)
{
	const vec3 n = NORMAL;
	const vec3 halfway = safeNormalize(wo + wi);
	const vec3 reflected = reflect(-wo, n);

	const float cos_theta_i = dot(n, wi);
	const float abs_cos_theta_i = abs(cos_theta_i);

	PBMaterialSampleData material;
	material.albedo = 1..xxx;
	material.alpha = 1;
	material.normal = vec3(0, 0, 1);
	material.flags = MATERIAL_FLAG_REFLECTION_BIT;
	material.metallic = ubo.metallic;
	material.roughness = ubo.roughness;

	GeometryShadingInfo geom;
	geom.position = 0..xxx;
	geom.geometry_normal = n;
	geom.vertex_shading_normal = geom.geometry_normal;
	geom.shading_normal = geom.vertex_shading_normal;
	geom.vertex_shading_tangent = TANGENT;

	float res = 0;

	bool apply_cos = true;

	if(index == 0)
	{
		if(cos_theta_i != 0)
		{
			res = evaluateBSDF(geom, wo, wi, material).x;
		}
	}
	else if (index == 1)
	{
		if(cos_theta_i > 0)
		{
			res = oo_PI;
		}
	}
	if(index == 2)
	{
		float shininess = ubo.shininess;
		shininess = pow(max(ubo.roughness, 0.01), -4.0);
		float diffuse_term = cos_theta_i > 0.0f ? oo_PI : 0.0f;
		float specular_term = EvaluateSpecularBSDF(geom.geometry_normal, wo, wi, shininess) / abs_cos_theta_i;

		float specular_weight = max(ubo.metallic, 0.04);

		res = lerp(diffuse_term, specular_term, specular_weight);
	}

	if(apply_cos)
	{
		res *= abs_cos_theta_i;
	}
	return res;
}


vec2 GetThreadUV(uvec2 thread_id, uvec2 dims)
{
	return vec2(thread_id) / vec2(dims - uvec2(1));
}

// x: azimuth
// y: inclination
uvec2 ModulateMaxPlusOne(uvec2 id, uvec2 resolution)
{
	uvec2 res = id;
	res.x = id.x % resolution.x;
	if(id.y == resolution.y)
	{
		res.y = resolution.y - 1;
		res.x += resolution.x / 2;
	}
	return res;
}


