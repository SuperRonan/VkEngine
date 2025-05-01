#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Random.slang>

#include "MaterialDefinitions.h"

enum MaterialType
{
	PhysicalyBased,
};

struct BSDFSample<int N>
{
	vec3 bsdf = {};
	float pdf = {};
	vec3 direction = {};
	uint flags = 0;
	Matrixf<3, N> direction_jacobian = Matrixf<3, N>::Zero();

	__init() {}
}

interface IMaterial
{
	uint getFlags();

	MaterialType getType();

	float3 bsdf(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in vec3 wo, 
		const in vec3 wi
	);

	BSDFSample<N> sampleBSDF<RNG:IRNG, int N>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in Matrixf<3, N> normal_jacobian,
		const in vec3 wo, 
		const in Matrixf<3, N> wo_jacobian,
		inout RNG rng, 
		uint flags = 0
	);
}

