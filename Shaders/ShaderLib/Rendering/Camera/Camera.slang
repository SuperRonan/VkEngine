#pragma once

#include <ShaderLib/common.slang>

#include <ShaderLib/Maths/View.slang>

#include "CameraDefinitions.h"

#include <ShaderLib/Rendering/Ray.slang>
#include <ShaderLib/Maths/AABB.slang>

#include <ShaderLib/Random.slang>

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
		return CameraType(FORCE_CAMERA_TYPE);
#else
		return CameraType((flags >> CAMERA_FLAGS_TYPE_BIT_OFFSET) & BIT_MASK(CAMERA_TYPE_BIT_COUNT));
#endif
	}
};

struct CameraRay
{
	Ray3f ray = {};
	RayRangef range = {};
	RayDiff3f differentials = {};

	__init()
	{

	}
};
struct CameraSample
{
	CameraRay ray = {};
	float pdf = 0;
	float importance = 0;

	__init()
	{

	}
};

struct CameraConnection
{
	Ray3f ray = {};
	RayRangef range = {};
	float importance = 0;
	float pdf = 0;
	vec2 uv = {};
	uint layer = 0;

	__init() {}
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

	CameraRay getRay(vec2 cp, Matrix2f cp_jacobian = Matrix2f::Zero(), uint layer = 0);

	CameraSample sampleWe<RNG : IRNG>(inout RNG rng, vec2 cp, Matrix2f cp_jacobian = Matrix2f::Zero(), uint layer = 0);

	CameraConnection sampleWi<RNG : IRNG>(inout RNG rng, vec3 position);
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



struct PerspectiveCamera : CameraBase, IMatrixCamera
{
	float z_near;
	float z_far;
	float inv_tan_half_fov;
	float inv_aspect;

	float getAspect()
	{
		return rcp(inv_aspect);
	}

	[mutating]
	void initPerspectiveCamera(const in StorageCamera cam)
	{
		initCameraBase(cam);

		z_near = cam.z_near;
		z_far = cam.z_far;
		inv_tan_half_fov = cam.inv_tan_half_fov_or_ortho_size;
		inv_aspect = cam.aspect_maybe_inv;
	}

	__init(const in StorageCamera cam)
	{
		initPerspectiveCamera(cam);
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

	CameraRay getRay(vec2 cp, Matrix2f cp_jacobian = Matrix2f::Zero(), uint layer = 0)
	{
		CameraRay res = {};
		res.ray.origin = position;
		mat3 B = (getBasis());
		const vec2 aspect = vec2(getAspect(), 1);
		const vec3 cam_dir = vec3(cp * aspect, inv_tan_half_fov);
		
		const vec3 cam_dir_n = Normalize(cam_dir);
		const vec3 d = B * cam_dir;
		const vec3 R = right;
		const vec3 U = getUp();
		const float d2 = Length2(cam_dir);
		vec3 rdx = (d2 * R + cam_dir.x * d) / pow(d2, 1.5);
		vec3 rdy = (d2 * U + cam_dir.y * d) / pow(d2, 1.5);
		res.differentials.direction_jacobian = MakeFromCols(rdx, rdy);
		res.differentials.direction_jacobian = res.differentials.direction_jacobian * (DiagonalMatrixV(aspect) * cp_jacobian);
		res.ray.direction = (B * cam_dir_n);
		res.range = RayRangef(vec2(z_near, z_far) / cam_dir_n.z);
		return res;
	}

	float getInvArea()
	{
		return 0.25 * inv_aspect;
	}

	CameraSample sampleWe<RNG : IRNG>(inout RNG rng, vec2 cp, Matrix2f cp_jacobian, uint layer)
	{
		CameraSample res;
		res.ray = getRay(cp, cp_jacobian);
		res.ray.range = {};
		const float ct = Dot(direction, res.ray.ray.direction);
		const float ct3 = ct * ct * ct;
		res.pdf = rcp(ct3) * getInvArea() * sqr(inv_tan_half_fov);
		res.importance = res.pdf;

		return res;
	}

	CameraConnection sampleWi<RNG : IRNG>(inout RNG rng, vec3 position)
	{
		// the length of the cp coordinate is linked to the cos theta, maybe we could exploit it for some optimization
		let w2p = getWorldToProj();
		const vec4 proj_h = w2p * Homogeneous(position);
		const vec3 proj = HomogeneousNormalize(proj_h);
		const bool in_frustum = proj_h.z > 0 && all(proj.xy <= 1..xx) && all(proj.xy >= -1..xx);
		CameraConnection res = {};
		if(in_frustum)
		{
			const vec3 d = this.position - position;
			const float d2 = Length2(d);
			res.ray.origin = position;
			res.ray.direction = Normalize(d);
			res.range.max = Length(d);
			res.range.min = 0;
			const float ct = -dot(res.ray.direction, this.direction);
			const float ct2 = sqr(ct);
			const float ct3 = ct2 * ct;
			// pdf_A(d) = delta_A
			// pdf_sa(d) = pdf_A(d) * d2 (Geometric density conversion)
			res.pdf = d2;
			// cost because their is an imaginary image plane, to compensate the natural optical vignetting
			// cost2 because rcp(cost) is distance to the image plane
			// sqr(inv_tan) to boost the signal to keep the same brightness with FOV changes (because the solid angle is the invert of the fov)
			res.importance = rcp(ct3) * getInvArea() * sqr(inv_tan_half_fov);
			res.uv = ClipSpaceToUV(proj.xy);
		}
		else
		{
			res.importance = 0;
			res.pdf = 1;
		}
		return res;
	}
};

// struct ReversePerspectiveCamera : CameraBase, ICamera
// {
// 	float z_near;
// 	float z_far;
// 	float inv_tan_half_fov;
// 	float inv_aspect;

// 	CameraType getType()
// 	{
// 		return CameraType::ReversePerspective;
// 	}

// 	bool hasInfiniteDepth()
// 	{
// #if FORCE_CAMERA_ZFAR
// 		return (FORCE_CAMERA_ZFAR == FORCE_CAMERA_ZFAR_INFINITE);
// #else
// 		return isinf(z_far);
// #endif
// 	}

// 	[mutating]
// 	void initReversePerspectiveCamera(const in StorageCamera cam)
// 	{
// 		initCameraBase(cam);

// 		z_near = cam.z_near;
// 		z_far = cam.z_far;
// 		inv_tan_half_fov = cam.inv_tan_half_fov_or_ortho_size;
// 		inv_aspect = cam.aspect_maybe_inv;
// 	}

// 	__init(const in StorageCamera cam)
// 	{
// 		initReversePerspectiveCamera(cam);
// 	}

// 	Ray3f getRay(vec2 cp, uint layer = 0)
// 	{
// 		Ray3f res;
// 		mat3 B = (getBasis());
		
// 		const vec3 cam_dir_1 = vec3(cp * vec2(rcp(inv_aspect), 1) * 2, 0);
// 		const vec3 cam_dir_2 = vec3(cp * vec2(rcp(inv_aspect), 1) * 1, inv_tan_half_fov);
// 		res.origin = position + B * cam_dir_1;
// 		res.direction = (B * Normalize(cam_dir_2 - cam_dir_1));
// 		return res;
// 	}
// };

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

	CameraRay getRay(vec2 cp, Matrix2f cp_jacobian, uint layer = 0)
	{
		CameraRay res = {};
		res.ray.direction = direction;
		res.ray.origin = position + frame_size * (cp.x * right + cp.y * getUp() * aspect);
		res.range = RayRangef(depth_range);
		res.differentials.origin_jacobian = MakeFromCols(
			right,
			getUp()
		) * cp_jacobian;
		return res;
	}

	CameraSample sampleWe<RNG : IRNG>(inout RNG rng, vec2 cp, Matrix2f cp_jacobian, uint layer)
	{
		CameraSample res;
		res.ray = getRay(cp, cp_jacobian, layer);
		res.pdf = 1.0f;
		res.importance = 1.0f;
		return res;
	}

	CameraConnection sampleWi<RNG : IRNG>(inout RNG rng, vec3 position)
	{
		CameraConnection res = {};
		res.pdf = 1.0f;
		vec3 pos_c = getWorldToView() * Homogeneous(position);
		vec2 cp = rcp(this.frame_size) * vec2(1, rcp(aspect)) * pos_c.xy;
		const bool in_frustum = pos_c.z > 0 && all(cp <= 1..xx) && all(cp >= -1..xx);
		if(in_frustum)
		{
			res.importance = 1;
			res.ray.origin = position;
			res.ray.direction = -this.direction;
			res.range = {};
			res.range.max = pos_c.z;
			res.importance = rcp(sqr(this.frame_size));
			res.uv = ClipSpaceToUV(cp);
		}
		else
		{
			res.importance = 0;
		}
		res.pdf = 1;
		return res;
	}
}

// Dynamic dispatch compile time resolve does not seem to complitely work yet
// So we need to do this
#ifdef FORCE_CAMERA_TYPE
#if FORCE_CAMERA_TYPE == CAMERA_TYPE_PERSPECTIVE
typealias ForcedCameraType = PerspectiveCamera;
#elif FORCE_CAMERA_TYPE == CAMERA_TYPE_ORTHO
typealias ForcedCameraType = OrthographicCamera;
#else // if FORCE_CAMERA_TYPE == CAMERA_TYPE_SPHERICAL
typealias ForcedCameraType = PerspectiveCamera;
#endif
#endif


IMatrixCamera MakeMatrixCamera(const in StorageCamera cam)
{
	const CameraType type = cam.getType();
	if(type == CameraType::Perspective)
	{
		return PerspectiveCamera(cam);
	}
	else //if(type == CameraType::Orthographic)
	{
		return OrthographicCamera(cam);
	}
}

ICamera MakeCamera(const in StorageCamera cam)
{
#ifdef FORCE_CAMERA_TYPE
	return ForcedCameraType(cam);
#else
	const CameraType type = cam.getType();
	if(type == CameraType::Perspective)
	{
		return PerspectiveCamera(cam);
	}
	else if(type == CameraType::Orthographic)
	{
		return OrthographicCamera(cam);
	}
	else// if(type == CameraType::Spherical)
	{
		return PerspectiveCamera(cam);
	}
#endif
}
