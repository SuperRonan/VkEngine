#pragma once

#include <ShaderLib:/common.glsl>

float microfacetD(float alpha2, vec3 normal, vec3 halfway)
{
	return alpha2 / (M_PI * sqr(sqr(dot(normal, halfway)) * (alpha2 - 1) + 1));
}

float microfacetGGX(vec3 n, vec3 v, float k)
{
	const float d = dot(n, v);
	return d / (d * (1.0 - k) + k);
}

float specularFresnelNormalIncidence(float n)
{
	return sqr(n - 1) / sqr(n + 1);
}

vec3 FresnelSchlick(vec3 F0, vec3 v, vec3 h)
{
	const float a = -5.55473f;
	const float b = -6.98316f;
	const float d = dot(v, h);
	return F0 + (1.0.xxx - F0) * pow(2, (a * d + b) * d);
}

// .x = specular shininess in R+ 
// .y = specular weight in [0, 1]
vec2 EstimatePhongParamsApprox(float cos_theta_o, float roughness, float metallic)
{
	vec2 res = vec2(0, 0);

	res.y = metallic;
	if(roughness == 0)
	{
		//res.x = 1.0f / 0.0f;
	}
	else
	{
		const float limit = 2e-2;
		if(roughness < limit)
		{
			roughness = limit;
		}
		res.x = pow(roughness, -4);
		//res.x = rcp(sqr(sqr(roughness)));
		//res.x = clamp(res.x, 1, 1e5);
	}

	return res;
}



float EvaluateShinyLobe(vec3 center_direction, vec3 direction, float shininess)
{
	const float m = (shininess + 1) / TWO_PI;
	const float c = max(0, dot(normalize(center_direction), normalize(direction)));
	const float p = pow(c, shininess);
	return m * p;
}

vec3 GenerateRandomSpecularDirection(float shininess, inout rng_t rng)
{
	const float x = randomFloat01(rng);
	const float y = randomFloat01(rng);
	const float p = pow(x, rcp(shininess + 1));
	const float theta = acos(p);
	const float phi = TWO_PI * y;
	vec3 res = SphericalToCartesian(vec2(theta, phi)).xzy;
	return res;
}
