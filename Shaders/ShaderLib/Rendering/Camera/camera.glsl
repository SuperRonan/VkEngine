#pragma once

#include <ShaderLib:/common.glsl>

#define CAMERA_TYPE_PERSPECTIVE 0
#define CAMERA_TYPE_ORTHO 1
#define CAMERA_TYPE_SPHERICAL 2
#define CAMERA_TYPE_BIT_COUNT 2
#define CAMERA_FLAGS_TYPE_BIT_OFFSET 0

#ifdef __cplusplus
#define STORAGE_MAT4x3 glm::mat4x3 
#else
#define STORAGE_MAT4x3 layout(row_major) mat4x3 
#endif

struct StorageCamera
{
	STORAGE_MAT4x3 world_to_camera; 
	STORAGE_MAT4x3 camera_to_world; 

	mat4 camera_to_proj;
	mat4 proj_to_camera;
	
	mat4 world_to_proj;
	mat4 world_to_camera;
	
	uint flags;
	uint extra_1;
	uint extra_2;
	uint extra_3;
};

mat4x3 ReadStorageMatrix(const in STORAGE_MAT4x3 m)
{
	return mat4x3(m);
}

vec3 GetCameraWorldPosition(const in StorageCamera cam)
{
	return cam.camera_to_world[3];
}

mat3 GetCameraWorldBasis(const in StorageCamera cam)
{
	return mat3(cam.camera_to_world);
}

uint GetCameraType(const in StorageCamera cam)
{
	return (cam.flags >> CAMERA_FLAGS_TYPE_BIT_OFFSET) & BIT_MASK(CAMERA_TYPE_BIT_COUNT)
}
