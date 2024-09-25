#pragma once

#include <ShaderLib:/common.glsl>

// https://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf

// Normal distribution
float microfacetD(float alpha2, vec3 normal, vec3 halfway)
{
	return alpha2 / (M_PI * sqr(sqr(dot(normal, halfway)) * (alpha2 - 1) + 1));
}

float microfacetGGX(vec3 n, vec3 v, float k)
{
	const float d = dot(n, v);
	return d / (d * (1.0 - k) + k);
}

// Shadowing masking function
float microfacetG(vec3 n, vec3 wo, vec3 wi, float k)
{
	const float gi = microfacetGGX(n, wi, k);
	const float go = microfacetGGX(n, wo, k);
	float res;
	res = gi * go;
	return res;
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

struct MicrofacetApproximation
{
	float specular_weight;
	float shininess;	
};

MicrofacetApproximation EstimateMicrofacetApprox(float cos_theta_o, float roughness, float metallic)
{
	MicrofacetApproximation res = MicrofacetApproximation(0, 0);

	res.specular_weight = max(metallic, 0.04);
	if(roughness == 0)
	{
		//res.x = 1.0f / 0.0f;
	}
	else
	{
		// const float soft_limit = 0.2;
		// if(roughness < (soft_limit))
		// {
		// 	//roughness = lerp(soft_limit * 0.35, soft_limit, roughness / soft_limit);
		// }
		const float limit = 2e-4;
		if(roughness < limit)
		{
			roughness = limit;
		}
		res.shininess = pow(roughness, -2);
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
