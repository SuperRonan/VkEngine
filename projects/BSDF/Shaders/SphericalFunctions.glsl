#pragma once

#include "Bindings.glsl"

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/Maths/transforms.glsl>

#include <RenderLibShaders:/RenderLib/shading.glsl>

#define NORMAL vec3(0, 1, 0)
#define TANGENT vec3(1, 0, 0)

float EvaluateShinyApprox(const in PBMaterialSampleData material, vec3 n, vec3 wo, vec3 wi)
{	
	const float cos_theta_i = dot(n, wi);
	const float abs_cos_theta_i = abs(cos_theta_i);
	const float cos_theta_o = dot(n, wo);

	const vec3 reflected = reflect(-wo, n);

	MicrofacetApproximation approx = EstimateMicrofacetApprox(cos_theta_o, material.roughness, material.metallic);

	float diffuse_term = max(cos_theta_i, 0) * oo_PI;
	float specular_term = EvaluateShinyLobe(reflected, wi, approx.shininess);

	float res = lerp(diffuse_term, specular_term, approx.specular_weight);
	return res;
}

float EvaluateSphericalFunction(const uint index, vec3 wo, vec3 wi)
{
	const vec3 n = NORMAL;
	const vec3 halfway = safeNormalize(wo + wi);
	const vec3 reflected = reflect(-wo, n);

	const float cos_theta_i = dot(n, wi);
	const float abs_cos_theta_i = abs(cos_theta_i);
	const float cos_theta_o = dot(n, wo);
	const float abs_cos_theta_o = abs(cos_theta_o);

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

	const float alpha2 = sqr(material.roughness);
	const float k = sqr(material.roughness + 1) / 8;
	const vec3 F0 = lerp(vec3(0.04), material.albedo, material.metallic);
	
	const float D = microfacetD(alpha2, n, halfway);
	const float G = microfacetG(n, wo, wi, k);
	const float F = FresnelSchlick(F0, wo, halfway).x;
	const float div = max(4.0f * abs_cos_theta_i * abs_cos_theta_o, EPSILON_f);
	
	float res = 0;

	bool apply_cos = true;

	if(index == 0)
	{
		res = evaluateBSDF(geom, wo, wi, material).x;
		if(cos_theta_i > 0)
		{
			res = D * F * G / div;
		}
	}
	else if (index == 1)
	{
		// if(cos_theta_i > 0)
		// {
		// 	res = oo_PI;
		// }
		float s = pow(material.roughness, -2 + 0.5);

		res = EvaluateShinyLobe(reflected, wi, s);

		apply_cos = false;
	}
	if(index == 2)
	{
		res = EvaluateShinyApprox(material, n, wo, wi);
		apply_cos = false;
	}
	if(index == 3)
	{
		float a = evaluateBSDF(geom, wo, wi, material).x * abs_cos_theta_i;
		float b = EvaluateShinyApprox(material, n, wo, wi);
		apply_cos = false;
		res = sqr(a - b);
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


