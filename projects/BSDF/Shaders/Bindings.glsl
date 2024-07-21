#pragma once

#include "SphericalFunctions.glsl"

#ifndef BSDF_BINDING_BASE
#define BSDF_BINDING_BASE MODULE_DESCRIPTOR_BINDING
#endif

layout(BSDF_BINDING_BASE + 0) uniform UBO
{
	mat4 camera_world_to_proj;

	vec3 direction;
	float common_alpha;

	vec4 colors[];
} ubo;

#ifndef BSDF_IMAGE_ACCESS
#define BSDF_IMAGE_ACCESS readonly
#endif

#ifndef BSDF_IMAGE_FORMAT
#define BSDF_IMAGE_FORMAT r32f
#endif

layout(BSDF_BINDING_BASE + 1, BSDF_IMAGE_FORMAT) uniform restrict image2DArray bsdf_image;
//layout(BSDF_BINDING_BASE + 1) uniform texture2D bsdf_texture;

