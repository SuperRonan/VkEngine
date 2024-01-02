#pragma once

#ifndef NUMBER_OF_CUBES
#define NUMBER_OF_CUBES 1

#endif
struct CubeState 
{
	vec3 center_position;
	vec3 center_velocity;
};

#define FRONT_BUFFER_FLAG 1

uint GetCurrentBufferIndex(uint cube_id, uint flags)
{
	const bool front_buffer = (flags & FRONT_BUFFER_FLAG) == 1;
	const uint buffer_index = (front_buffer ? 0 : NUMBER_OF_CUBES) + cube_id;
	return buffer_index;
}

uint GetPrevBufferIndex(uint cube_id, uint flags)
{
	const bool front_buffer = (flags & FRONT_BUFFER_FLAG) == 1;
	const uint buffer_index = (front_buffer ? NUMBER_OF_CUBES : 0) + cube_id;
	return buffer_index;
}


