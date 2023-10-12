#pragma once

#include "Material.glsl"

struct PBMaterialProperties
{
	vec3 albedo;
	uint flags;
};

#ifndef BIND_SINGLE_MATERIAL
#define BIND_SINGLE_MATERIAL 1
#endif

#if BIND_SINGLE_MATERIAL

layout(INSTANCE_DESCRIPTOR_BINDING + MATERIAL_BINDING_BASE + 0, std140) uniform restrict MaterialProps
{
	PBMaterialProperties props;
} material_props;


#endif