#pragma once

#include <ShaderLib:/common.glsl>

#ifndef BSDF_BINDING_BASE
#define BSDF_BINDING_BASE MODULE_DESCRIPTOR_BINDING
#endif

layout(BSDF_BINDING_BASE + 0) uniform UBO
{
	mat4 camera_world_to_proj;
	mat4 camera_world_to_camera;

	vec3 direction;
	float common_alpha;

	uint reference_function;
	uint seed;
	uint pad1, pad2;

	float roughness;
	float metallic;
	float shininess;
	float pad3;
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
	return mat4x3(ubo.camera_world_to_camera);
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

#define IMPORTANCE_SAMPLING_METHOD_UNIFORM 0
#define IMPORTANCE_SAMPLING_METHOD_COSINE 1

#ifndef DEFAULT_IMPORTANCE_SAMPLING_METHOD
#define DEFAULT_IMPORTANCE_SAMPLING_METHOD IMPORTANCE_SAMPLING_METHOD_COSINE
#endif

#ifndef STATISTICS_BUFFER_ACCESS 
#define STATISTICS_BUFFER_ACCESS readonly
#endif

struct FunctionStatistics
{
	float integral;
	float variance_with_reference;
	float pad1, pad2;
};

layout(BSDF_BINDING_BASE + 3) restrict STATISTICS_BUFFER_ACCESS buffer StatisticsBufferBinding
{
	FunctionStatistics stats[];
} _statistics;

