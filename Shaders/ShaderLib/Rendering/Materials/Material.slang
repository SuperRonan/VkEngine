#pragma once

#include <ShaderLib/common.slang>

#include "MaterialDefinitions.h"

enum MaterialType
{
	PhysicalyBased,
};

interface IMaterial
{
	uint getFlags();

	MaterialType getType();

	float3 bsdf(const in vec3 geometry_normal, const in vec3 shading_normal, const in vec3 wo, const in vec3 wi);
}

