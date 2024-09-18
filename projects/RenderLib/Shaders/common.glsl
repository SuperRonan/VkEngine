#pragma once

#include <ShaderLib:/common.glsl>

#include <ShaderLib:/Rendering/Camera/camera.glsl>

#ifndef RENDERER_BINDING
#define RENDERER_BINDING MODULE_DESCRIPTOR_BINDING + 0
#endif

#define RENDERER_BINDING_COUNT 1

layout(RENDERER_BINDING + 0) uniform UBO
{
	float time;
	float delta_time;
	uint frame_idx;

	StorageCamera camera;
} ubo;


layout(RENDERER_BINDING + 1) uniform samplerShadow LightDepthSampler;
