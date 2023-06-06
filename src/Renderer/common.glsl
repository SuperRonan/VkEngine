#pragma once



#ifndef UBO_BINDING
#define UBO_BINDING set = 0, binding = 0
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

