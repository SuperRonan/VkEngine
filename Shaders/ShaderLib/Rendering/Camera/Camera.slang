#pragma once

#include <ShaderLib/common.slang>

#include <ShaderLib/Maths/View.slang>

#include "CameraDefinitions.h"

#include <ShaderLib/Rendering/Ray.slang>
#include <ShaderLib/Maths/AABB.slang>

enum CameraType
{
	Perspective = CAMERA_TYPE_PERSPECTIVE,
	Orthographic = CAMERA_TYPE_ORTHO,
	ReversePerspective = CAMERA_TYPE_REVERSE_PERSPECTIVE,
	Spherical = CAMERA_TYPE_SPHERICAL,
}

struct StorageCamera
{ 
	// The direction vectors should orthonormals
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

	CameraType getType()
	{
#ifdef FORCE_CAMERA_TYPE
		return FORCE_CAMERA_TYPE;
#else
		return CameraType((flags >> CAMERA_FLAGS_TYPE_BIT_OFFSET) & BIT_MASK(CAMERA_TYPE_BIT_COUNT));
#endif
	}
};

interface ICamera
{
	__init(const in StorageCamera cam);

	CameraType getType();

	uint getFlags();

	vec3 getPosition();

	// [right, up, front]
	mat3 getBasis();

	AffineXForm3Df getWorldToView(uint layer = 0);

	AffineXForm3Df getViewToWorld(uint layer = 0);

	bool hasInfiniteDepth();

	Ray3f getRay(vec2 cp, uint layer = 0);
}

interface IMatrixCamera : ICamera
{
	__init(const in StorageCamera cam);

	mat4 getViewToProj(uint layer = 0);

	mat4 getProjToView(uint layer = 0);

	mat4 getWorldToProj(uint layer = 0);

	mat4 getProjToWorld(uint layer = 0);
}

struct CameraBase
{
	vec3 position;
	uint flags;
	
	vec3 direction;
	vec3 right;

	[mutating]
	void initCameraBase(const in StorageCamera cam)
	{
		position = cam.position;
		flags = cam.flags;
		direction = cam.direction;
		right = cam.right;
	}

	uint getFlags()
	{
		return flags;
	}

	vec3 getPosition()
	{
		return position;
	}

	vec3 getUp()
	{
		return (Cross(direction, right));
	}

	mat3 getBasis()
	{
		return MakeFromCols(right, getUp(), direction);
	}

	AffineXForm3Df getWorldToView(uint layer = 0)
	{
		// TODO re-evaluate wether it makes sense to invert the Y-Axis here? It could be done in the projection matrix
		// (The function takes a "down" vector, but we give it a "up" vector)
		return LookAtDirAssumeOrtho(position, direction, getUp(), right);
	}

	AffineXForm3Df getViewToWorld(uint layer = 0)
	{
		return InverseLookAtDirAssumeOrtho(position, direction, getUp(), right);
	}
}

struct CameraRay
{
	Ray3fAndRange ray;
	vec3 rdx;
	vec3 rdy;

	[mutating]
	void scaleDifferentials(vec2 scale)
	{
		rdx *= scale.x;
		rdy *= scale.y;
	}
}

struct PerspectiveCamera : CameraBase, IMatrixCamera
{
	float z_near;
	float z_far;
	float inv_tan_half_fov;
	float inv_aspect;

	[mutating]
	void initPerspectiveCamera(const in StorageCamera cam, vec2 oo_dims = vec2(0))
	{
		initCameraBase(cam);

		z_near = cam.z_near;
		z_far = cam.z_far;
		inv_tan_half_fov = cam.inv_tan_half_fov_or_ortho_size;
		inv_aspect = cam.aspect_maybe_inv;
	}

	__init(const in StorageCamera cam, vec2 oo_dims = vec2(0))
	{
		initPerspectiveCamera(cam, oo_dims);
	}


	CameraType getType()
	{
		return CameraType::Perspective;
	}

	bool hasInfiniteDepth()
	{
#if FORCE_CAMERA_ZFAR
		return (FORCE_CAMERA_ZFAR == FORCE_CAMERA_ZFAR_INFINITE);
#else
		return isinf(z_far);
#endif
	}

	mat4 getViewToProj(uint layer = 0)
	{
		if(hasInfiniteDepth())
		{
			return InfinitePerspectiveProjFromInvTanInvAspect(inv_tan_half_fov, inv_aspect, z_near);
		}
		else
		{
			return PerspectiveProjFromInvTanInvAspect(inv_tan_half_fov, inv_aspect, vec2(z_near, z_far));
		}
	}

	mat4 getProjToView(uint layer = 0)
	{
		if(hasInfiniteDepth())
		{
			return InverseInfinitePerspectiveProjFromTanInvZnear(rcp(inv_tan_half_fov), rcp(inv_aspect), rcp(z_near));
		}
		else
		{
			return InversePerspectiveProjFromTan(rcp(inv_tan_half_fov), rcp(inv_aspect), rcp(vec2(z_near, z_far)));
		}
	}

	mat4 getWorldToProj(uint layer = 0)
	{
		return getViewToProj(layer) * ResizeMatrix<4, 4>(getWorldToView(layer));
	}

	mat4 getProjToWorld(uint layer = 0)
	{
		return ResizeMatrix<4, 4>(getViewToWorld(layer)) * getProjToView(layer);
	}

	Ray3f getRay(vec2 cp, uint layer = 0)
	{
		Ray3f res;
		res.origin = position;
		mat3 B = (getBasis());
		const vec3 cam_dir = vec3(cp * vec2(rcp(inv_aspect), 1), inv_tan_half_fov);
		res.direction = (B * Normalize(cam_dir));
		return res;
	}

	CameraRay getFullRay(vec2 cp)
	{
		CameraRay res;
		res.ray.origin = position;
		mat3 B = (getBasis());
		const vec3 cam_dir = vec3(cp * vec2(rcp(inv_aspect), 1), inv_tan_half_fov);
		const vec3 cam_dir_n = Normalize(cam_dir);
		const vec3 d = B * cam_dir;
		res.rdx = (Length2(d) * right + Dot(d, right) * d) / pow(Length2(d), 1.5);
		res.rdy = (Length2(d) * getUp() + Dot(d, getUp()) * d) / pow(Length2(d), 1.5);
		res.ray.direction = (B * cam_dir_n);
		res.ray.range = vec2(z_near, z_far) / cam_dir_n.z;
		return res;
	}
};

struct ReversePerspectiveCamera : CameraBase, ICamera
{
	float z_near;
	float z_far;
	float inv_tan_half_fov;
	float inv_aspect;

	CameraType getType()
	{
		return CameraType::ReversePerspective;
	}

	bool hasInfiniteDepth()
	{
#if FORCE_CAMERA_ZFAR
		return (FORCE_CAMERA_ZFAR == FORCE_CAMERA_ZFAR_INFINITE);
#else
		return isinf(z_far);
#endif
	}

	[mutating]
	void initReversePerspectiveCamera(const in StorageCamera cam)
	{
		initCameraBase(cam);

		z_near = cam.z_near;
		z_far = cam.z_far;
		inv_tan_half_fov = cam.inv_tan_half_fov_or_ortho_size;
		inv_aspect = cam.aspect_maybe_inv;
	}

	__init(const in StorageCamera cam)
	{
		initReversePerspectiveCamera(cam);
	}

	Ray3f getRay(vec2 cp, uint layer = 0)
	{
		Ray3f res;
		mat3 B = (getBasis());
		
		const vec3 cam_dir_1 = vec3(cp * vec2(rcp(inv_aspect), 1) * 2, 0);
		const vec3 cam_dir_2 = vec3(cp * vec2(rcp(inv_aspect), 1) * 1, inv_tan_half_fov);
		res.origin = position + B * cam_dir_1;
		res.direction = (B * Normalize(cam_dir_2 - cam_dir_1));
		return res;
	}
};

struct ThinLensCamera : PerspectiveCamera
{
	float aperture;
	uint shape;
	float shape_rotation;
	float focal_distance;

	[mutating]
	void initThinLens(const in StorageCamera cam)
	{
		initPerspectiveCamera(cam);
		aperture = cam.aperture;
		shape = FORCE_CAMERA_APERTURE_SHAPE;
		shape_rotation = FORCE_CAMERA_APERTURE_ROTATION;
		focal_distance = cam.focal_distance;
	}

	__init(const in StorageCamera cam)
	{
		initThinLens(cam);
	}

	float getFocalLength()
	{
		return rcp(rcp(inv_tan_half_fov) + rcp(focal_distance));
	}
}


struct OrthographicCamera : CameraBase, IMatrixCamera
{
	float frame_size;
	float aspect;
	vec2 depth_range;

	CameraType getType()
	{
		return CameraType::Orthographic;
	}

	[mutating]
	void initOrthographic(const in StorageCamera cam)
	{
		initCameraBase(cam);

		frame_size = cam.inv_tan_half_fov_or_ortho_size;
		aspect = cam.aspect_maybe_inv;
		depth_range = vec2(cam.z_near, cam.z_far);
	}

	__init(const in StorageCamera cam)
	{
		initOrthographic(cam);
	}

	// AABB in view space
	AABB3f getAABB()
	{
		AABB3f res;
		const vec2 frame = frame_size * vec2(aspect, 1);
		res._bottom = vec3(-frame, depth_range.x);
		res._top = vec3(frame, depth_range.y);
		return res;
	}

	mat4 getViewToProj(uint layer = 0)
	{
		AABB3f volume = getAABB();
		return OrthoProj(volume.bottom(), volume.top());
	}

	mat4 getProjToView(uint layer = 0)
	{
		AABB3f volume = getAABB();
		return InverseOrthoProj(volume.bottom(), volume.top());
	}

	mat4 getWorldToProj(uint layer = 0)
	{
		return getViewToProj(layer) * ResizeMatrix<4, 4>(getWorldToView(layer));
	}

	mat4 getProjToWorld(uint layer = 0)
	{
		return ResizeMatrix<4, 4>(getViewToWorld(layer)) * getProjToView(layer);
	}

	bool hasInfiniteDepth()
	{
		return false;
	}

	Ray3f getRay(vec2 cp, uint layer = 0)
	{
		Ray3f res;
		res.direction = direction;
		res.origin = position + cp.x * right + cp.y * getUp();
		return res;
	}
}


IMatrixCamera MakeMatrixCamera(const in StorageCamera cam)
{
	const CameraType type = cam.getType();
	if(type == CameraType::Perspective)
	{
		return PerspectiveCamera(cam);
	}
	else if(type == CameraType::Orthographic)
	{
		return OrthographicCamera(cam);
	}
}
