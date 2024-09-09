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

