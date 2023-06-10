#pragma once

#ifndef RENDERER_FIRST_BINDING
#define RENDERER_FIRST_BINDING set = 1, binding = 0
#endif

#ifndef UBO_BINDING
#define UBO_BINDING RENDERER_FIRST_BINDING
#endif


layout(UBO_BINDING) uniform UBO
{
	float time;
	float delta_time;
	uint frame_idx;

	mat4 world_to_camera;
	mat4 camera_to_proj;
	mat4 world_to_proj;
} ubo;

