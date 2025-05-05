#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Random.slang>

#include "MaterialDefinitions.h"

enum MaterialType
{
	PhysicalyBased,
};

[Flags]
enum BSDFSampleFlags
{
	None = 0x0,
	Delta = 0x1,
	Symmetric = 0x2,
};

struct BSDFSample<int N>
{
	typealias Flags = BSDFSampleFlags;
	vec3 bsdf = {};
	float pdf = {};
	vec3 direction = {};
	Flags flags = Flags::None;
	Matrixf<3, N> direction_jacobian = Matrixf<3, N>::Zero();

	__init() {}
}

#define BSDF_FORWARD_BIT 0
#define BSDF_ADJOINT_BIT 1

interface IMaterial
{
	uint getFlags();

	MaterialType getType();

	float3 bsdf<int Flags>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in vec3 wo, 
		const in vec3 wi
	);

	float pdf<int Flags>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in vec3 wo, 
		const in vec3 wi
	);

	BSDFSample<N> sampleBSDF<int Flags, typename RNG : IRNG, int N>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in Matrixf<3, N> Jnormal,
		const in vec3 wo, 
		const in Matrixf<3, N> Jwo,
		inout RNG rng, 
		uint flags = 0
	);
}

// __generic<typename Material : IMaterial>
// extension Material
// {
// 	float3 bsdf(
// 		const in vec3 geometry_normal, 
// 		const in vec3 shading_normal, 
// 		const in vec3 wo, 
// 		const in vec3 wi
// 	) {
// 		return this.bsdf<BSDF_FORWARD_BIT>(
// 			geometry_normal,
// 			shading_normal,
// 			wo,
// 			wi
// 		);
// 	}

// 	float pdf(
// 		const in vec3 geometry_normal, 
// 		const in vec3 shading_normal, 
// 		const in vec3 wo, 
// 		const in vec3 wi
// 	) {
// 		return this.pdf<BSDF_FORWARD_BIT>(
// 			geometry_normal,
// 			shading_normal,
// 			wo,
// 			wi
// 		);
// 	}

// 	BSDFSample<N> sampleBSDF<RNG:IRNG, int N>(
// 		const in vec3 geometry_normal, 
// 		const in vec3 shading_normal, 
// 		const in Matrixf<3, N> Jnormal,
// 		const in vec3 wo, 
// 		const in Matrixf<3, N> Jwo,
// 		inout RNG rng, 
// 		uint flags = 0
// 	) {
// 		return this.sampleBSDF<BSDF_FORWARD_BIT>(
// 			geometry_normal,
// 			shading_normal,
// 			Jnormal,
// 			wo, Jwo,
// 			rng,
// 			flags
// 		);
// 	}

// 	BSDFSample<2> sampleBSDF<RNG:IRNG>(
// 		const in vec3 geometry_normal, 
// 		const in vec3 shading_normal, 
// 		const in vec3 wo, 
// 		inout RNG rng, 
// 		uint flags = 0
// 	) {
// 		return this.sampleBSDF<BSDF_FORWARD_BIT>(
// 			geometry_normal,
// 			shading_normal,
// 			Matrixf<3, 2>::Zero(),
// 			wo, Matrixf<3, 2>::Zero(),
// 			rng,
// 			flags
// 		);
// 	}
// };

