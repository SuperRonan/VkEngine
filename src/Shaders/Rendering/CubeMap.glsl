#pragma once

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/Maths/transforms.glsl>

vec3 cubeMapFaceDirection(uint id)
{
	vec3 res = vec3(0);
	res[id / 2] = ((id % 2) == 0) ? 1 : -1;
	return res;
}

vec3 cubeMapFaceUpDirection(uint id)
{
	vec3 res = cubeMapFaceDirection((id + 2) % 6);
	switch(id)
	{
		case 0:
		case 1:
		case 4:
		case 5:
			res = vec3(0, 1, 0);
		break;
		case 2:
			res = vec3(0, 0, -1);
		break;
		case 3:
			res = vec3(0, 0, 1);
		break; 
	}
	return res;
}

mat4 cubeMapFaceProjection(uint id, vec3 position, float z_near)
{
	const vec3 front = cubeMapFaceDirection(id);
	const vec3 up = cubeMapFaceUpDirection(id);
	mat4 res = infinitePerspectiveProjFromFOV(HALF_PI, 1, z_near) * mat4(lookAtAssumeNormalized4x3(position, -front, -up));
	return res;
}

uint findCubeDirectionId(vec3 direction)
{
	int max_dim = 0;
	float max_value = abs(direction.x); 
	for(int i=1; i<3; ++i)
	{
		if(abs(direction[i]) > max_value)
		{
			max_dim = i;
			max_value = abs(direction[i]);
		}
	}
	uint res = max_dim * 2;
	if(direction[max_dim] < 0)
		res |= 1;

	return res;
}

vec3 quantifyOnCubeDirection(vec3 direction)
{
	int max_dim = 0;
	float max_value = abs(direction.x); 
	for(int i=1; i<3; ++i)
	{
		if(abs(direction[i]) > max_value)
		{
			max_dim = i;
			max_value = abs(direction[i]);
		}
	}
	vec3 res = 0..xxx;
	res[max_dim] = sign(direction[max_dim]);
	return res;
}

vec4 cubeMapDirectionAndDepth(vec3 position, vec3 center, float z_near)
{
	vec3 direction = position - center;
	uint cube_id = findCubeDirectionId(direction);
	mat4 matrix = cubeMapFaceProjection(cube_id, center, z_near);
	vec4 h_pos = matrix * vec4(position, 1);
	float depth = h_pos.z / h_pos.w;
	vec4 res;
	res.xyz = cubeMapFaceDirection(cube_id);
	res.w = depth;
	return res;
}

float cubeMapDepth(vec3 position, vec3 center, float z_near)
{
	return cubeMapDirectionAndDepth(position, center, z_near).w;
}