#pragma once

#include <ShaderLib:/common.glsl>

#include <ShaderLib:/Maths/transforms.glsl>

#define CAMERA_TYPE_PERSPECTIVE 0
#define CAMERA_TYPE_ORTHO 1
#define CAMERA_TYPE_REVERSE_PERSPECTIVE 2
#define CAMERA_TYPE_SPHERICAL 3
#define CAMERA_TYPE_BIT_COUNT 2
#define CAMERA_FLAGS_TYPE_BIT_OFFSET 0

#ifndef FORCE_CAMERA_INFINITE_ZFAR
#define FORCE_CAMERA_INFINITE_ZFAR 0
#endif

#ifndef FORCE_CAMERA_APERTURE_SHAPE
#define FORCE_CAMERA_APERTURE_SHAPE 6
#endif

#ifndef FORCE_CAMERA_APERTURE_ROTATION
#define FORCE_CAMERA_APERTURE_ROTATION (0.0 * M_PI / 180.0)
#endif

struct StorageCamera
{ 
	// The direction vectors should be normalized
	// dot(direction, right) == 0
	vec3 position;
	float z_near;

	vec3 direction;
	float z_far;
	
	vec3 right;
	uint flags;
	
	float inv_tan_half_fov_or_ortho_size;
	float aspect_maybe_inv;
	float aperture;
	float focal_distance;
};

uint GetCameraType(const in StorageCamera cam)
{
#ifdef FORCE_CAMERA_TYPE
	return FORCE_CAMERA_TYPE;
#else
	return (cam.flags >> CAMERA_FLAGS_TYPE_BIT_OFFSET) & BIT_MASK(CAMERA_TYPE_BIT_COUNT);
#endif
}

vec3 GetCameraWorldPosition(const in StorageCamera cam)
{
	return cam.position;
}

vec3 GetCameraFrontDirection(const in StorageCamera cam)
{
	return cam.direction;
}

vec3 GetCameraRightDirection(const in StorageCamera cam)
{
	return cam.right;
}

vec3 GetCameraUpDirection(const in StorageCamera cam)
{
	const vec3 up = cross(cam.direction, cam.right);
	return up;
}

// [Right, Up, Front]
mat3 GetCameraWorldBasis(const in StorageCamera cam)
{	
	mat3 m = mat3(GetCameraRightDirection(cam), GetCameraUpDirection(cam), GetCameraFrontDirection(cam));
	return m;
}

bool CameraHasInfiniteDepth(const in StorageCamera cam)
{
	return (FORCE_CAMERA_INFINITE_ZFAR != 0) || isinf(cam.z_far);
}

mat4x3 GetCameraWorldToCam(const in StorageCamera cam)
{
	const vec3 up = cross(cam.direction, cam.right);
	return LookAtDir4x3AssumeOrtho(cam.position, cam.direction, up, cam.right);
}

mat4x3 GetCameraCamToWorld(const in StorageCamera cam)
{
	return InverseRigidTransform(GetCameraWorldToCam(cam));
}

float GetOrthoCameraAspect(const in StorageCamera cam)
{
	return cam.aspect_maybe_inv;
}

float GetPerspectiveCameraInvAspect(const in StorageCamera cam)
{
	return cam.aspect_maybe_inv;
}

float GetPerspectiveCameraInvTan(const in StorageCamera cam)
{
	return cam.inv_tan_half_fov_or_ortho_size;
}

float GetOrthoCameraFrameSize(const in StorageCamera cam)
{
	return cam.inv_tan_half_fov_or_ortho_size;
}

float GetPerspectiveCameraApertureRadius(const in StorageCamera cam)
{
	return cam.aperture;
}

uint GetPerspectiveCameraApertureShape(const in StorageCamera cam)
{
	return FORCE_CAMERA_APERTURE_SHAPE;
}

float GetPerspectiveCameraApertureRotation(const in StorageCamera cam)
{
	return FORCE_CAMERA_APERTURE_ROTATION;
}

float GetPerspectiveCameraFocalDistance(const in StorageCamera cam)
{
	return cam.focal_distance;
}

float GetPerspectiveCameraFocalLength(const in StorageCamera cam)
{
	return rcp(rcp(GetPerspectiveCameraInvTan(cam)) + rcp(GetPerspectiveCameraFocalDistance(cam)));
}

float GetPerspectiveCameraDistanceFilmLens(const in StorageCamera cam)
{
	return GetPerspectiveCameraInvTan(cam);
}

mat2x3 GetCameraOrthoAABB(const in StorageCamera cam)
{
	mat2x3 res;
	const vec2 frame = GetOrthoCameraFrameSize(cam) * vec2(GetOrthoCameraAspect(cam), 1);
	res[0] = vec3(-frame, cam.z_near);
	res[1] = vec3(frame, cam.z_far);
	return res;
}

mat4 GetCameraCamToProj(const in StorageCamera cam)
{
	const uint type = GetCameraType(cam);
	mat4 res;
	if(type == CAMERA_TYPE_PERSPECTIVE)
	{
		const bool infinite = CameraHasInfiniteDepth(cam);
		const float inv_aspect = GetPerspectiveCameraInvAspect(cam);
		const float inv_tan = GetPerspectiveCameraInvTan(cam);
		if(infinite)
		{
			res = InfinitePerspectiveProjFromInvTanInvAspect(inv_tan, inv_aspect, cam.z_near);
		}
		else
		{
			res = PerspectiveProjFromInvTanInvAspect(inv_tan, inv_aspect, vec2(cam.z_near, cam.z_far));
		}
	}
	else if(type == CAMERA_TYPE_ORTHO)
	{
		const mat2x3 volume = GetCameraOrthoAABB(cam);
		res = OrthoProj(volume[0], volume[1]);
	}
	return res;
}

mat4 GetCameraProjToCam(const in StorageCamera cam)
{
	const uint type = GetCameraType(cam);
	mat4 res;
	if(type == CAMERA_TYPE_PERSPECTIVE)
	{
		const bool infinite = CameraHasInfiniteDepth(cam);
		const float inv_aspect = GetPerspectiveCameraInvAspect(cam);
		const float inv_tan = GetPerspectiveCameraInvTan(cam);
		if(infinite)
		{
			res = InverseInfinitePerspectiveProjFromTan(rcp(inv_tan), rcp(inv_aspect), cam.z_far);
		}
		else
		{
			res = InversePerspectiveProjFromTan(rcp(inv_tan), rcp(inv_aspect), vec2(cam.z_near, cam.z_far));
		}
	}
	else if(type == CAMERA_TYPE_ORTHO)
	{
		const mat2x3 volume = GetCameraOrthoAABB(cam);
		res = InverseOrthoProj(volume[0], volume[1]);
	}
	return res;
}

mat4 GetCameraWorldToProj(const in StorageCamera cam)
{
	return GetCameraCamToProj(cam) * mat4(GetCameraWorldToCam(cam));
}

mat4 GetCameraProjToWorld(const in StorageCamera cam)
{
	return mat4(GetCameraCamToWorld(cam)) * GetCameraProjToCam(cam);
}