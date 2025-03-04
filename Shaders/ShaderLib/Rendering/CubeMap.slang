#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/View.slang>

vec3 CubeMapFaceDirection(uint id)
{
	vec3 res = vec3(0);
	res[id / 2] = ((id % 2) == 0) ? 1 : -1;
	return res;
}

vec3 CubeMapFaceUpDirection(uint id)
{
	vec3 res;
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

AffineXForm3Df GetCubeMapFaceWorldToView(uint id, vec3 position)
{
	const vec3 front = CubeMapFaceDirection(id);
	const vec3 up = CubeMapFaceUpDirection(id);
	return LookAtDirAssumeOrtho(position, front, up);
}

AffineXForm3Df GetCubeMapFaceViewToWorld(uint id, vec3 position)
{
	const vec3 front = CubeMapFaceDirection(id);
	const vec3 up = CubeMapFaceUpDirection(id);
	return InverseLookAtDirAssumeOrtho(position, front, up);
}

mat4 GetCubeMapViewToProj(float z_near)
{
	return InfinitePerspectiveProjFromFOV(HALF_PI, 1, z_near);
}

mat4 GetCubeMapProjToView(float z_near)
{
	return InverseInfinitePerspectiveProjFromFOV(HALF_PI, 1, z_near);
}

mat4 GetCubeMapFaceWorldToProj(uint id, vec3 position, float z_near)
{
	const mat4 res = GetCubeMapViewToProj(z_near) * ResizeMatrix<4, 4>(GetCubeMapFaceWorldToView(id, position));
	return res;
}

mat4 GetCubeMapFaceProjToWorld(uint id, vec3 position, float z_near)
{
	const mat4 res = ResizeMatrix<4, 4>(GetCubeMapFaceViewToWorld(id, position)) * GetCubeMapProjToView(z_near);
	return res;
}

uint FindCubeDirectionId(vec3 direction)
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

vec3 QuantifyOnCubeDirection(vec3 direction)
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
	res[max_dim] = float(sign(direction[max_dim]));
	return res;
}

vec4 CubeMapDirectionAndDepth(vec3 position, vec3 center, float z_near)
{
	vec3 direction = position - center;
	uint cube_id = FindCubeDirectionId(direction);
	mat4 matrix = GetCubeMapFaceWorldToProj(cube_id, center, z_near);
	vec4 h_pos = matrix * vec4(position, 1);
	float depth = h_pos.z / h_pos.w;
	vec4 res;
	res.xyz = CubeMapFaceDirection(cube_id);
	res.w = depth;
	return res;
}

float CubeMapDepth(vec3 position, vec3 center, float z_near)
{
	return CubeMapDirectionAndDepth(position, center, z_near).w;
}
