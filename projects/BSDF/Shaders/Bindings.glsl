#pragma once

#include "SphericalFunctions.glsl"

#ifndef BSDF_BINDING_BASE
#define BSDF_BINDING_BASE MODULE_DESCRIPTOR_BINDING
#endif

layout(BSDF_BINDING_BASE + 0) uniform UBO
{
	mat4 camera_world_to_proj;
	mat3x4 camera_world_to_camera;

	vec3 direction;
	float common_alpha;
} ubo;

#ifndef BSDF_COLOR_ACCESS
#define BSDF_COLOR_ACCESS readonly
#endif

layout(BSDF_BINDING_BASE + 1) restrict BSDF_COLOR_ACCESS buffer Colors
{
	vec4 colors[];
} functions_colors;

vec4 GetColor(uint layer)
{
	return functions_colors.colors[layer];
}

mat4x3 GetWorldToCamera()
{
	return transpose(ubo.camera_world_to_camera);
}

#ifndef BSDF_IMAGE_ACCESS
#define BSDF_IMAGE_ACCESS readonly
#endif

#ifndef BSDF_IMAGE_FORMAT
#define BSDF_IMAGE_FORMAT r32f
#endif

layout(BSDF_BINDING_BASE + 2, BSDF_IMAGE_FORMAT) uniform restrict image2DArray bsdf_image;
//layout(BSDF_BINDING_BASE + 3) uniform texture2D bsdf_texture;

#define BSDF_RENDER_MODE_UNIFORM 0
#define BSDF_RENDER_MODE_TRANSPARENT 1
#define BSDF_RENDER_MODE_WIREFRAME 2

#ifndef BSDF_RENDER_MODE
#define BSDF_RENDER_MODE BSDF_RENDER_MODE_TRANSPARENT
#endif

#ifndef BSDF_RENDER_USE_TEXTURE
#define BSDF_RENDER_USE_TEXTURE 0
#endif