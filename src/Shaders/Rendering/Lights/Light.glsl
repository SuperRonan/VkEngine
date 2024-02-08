#pragma once

#include <ShaderLib:/common.glsl>

#define LIGHT_TYPE_POINT (0x1)
#define LIGHT_TYPE_DIRECTIONAL (0x2)
#define LIGHT_TYPE_SPOT (0x3)
#define LIGHT_TYPE_MASK (0x7)

#define SPOT_LIGHT_FLAGS_POS 8
#define SPOT_LIGHT_FLAGS_MASK (0b1111 << SPOT_LIGHT_FLAGS_POS)
#define SPOT_LIGHT_FLAG_ATTENUATION (0x1 << SPOT_LIGHT_FLAGS_POS)


struct Light
{
	vec3 position;
	uint flags;
	vec3 emission;
	int pad;
	uvec4 textures;
	mat4 matrix;
};
