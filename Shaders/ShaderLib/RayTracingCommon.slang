#pragma once

#include "common.slang"

#include "Rendering/Ray.slang"

#include <ShaderLib/Maths/NumericLimits.slang>

#if SHADER_RAY_QUERY_AVAILABLE || SHADER_SEMANTIC_RAY_TRACING_PIPELINE
#define CAN_BIND_TLAS 1
#extension GL_EXT_ray_flags_primitive_culling : require
#else
#define CAN_BIND_TLAS 0
#endif

#if SHADER_RAY_TRACING_POSITION_FETCH_AVAILABLE
#extension GL_EXT_ray_tracing_position_fetch : require
#endif

#if SHADER_RAY_QUERY_AVAILABLE
#extension GL_EXT_ray_query : require
#endif

#if SHADER_RAY_TRACING_AVAILABLE
#extension GL_EXT_ray_tracing : require
#endif

typealias TLAS_t = RaytracingAccelerationStructure;

// TODO https://developer.nvidia.com/blog/solving-self-intersection-artifacts-in-directx-raytracing/

template <typename T : __BuiltinArithmeticType, int N>
T RayTMin(const in Vector<T, N> origin)
{
	// Not perfect, but good enough
	T res = Max(Abs(origin));
	if(IS_BUILTIN_FLOAT(T))
	{
		T div = T((1u << (sizeof(T) * 5u)));
		res = res / div;
		res = Max(res, NumericLimits<T>::min() * T(64));
	}
	return res;
}

template <typename T : __BuiltinArithmeticType, int N>
T RayTMin(const in Vector<T, N> origin, T cos_theta)
{
	// Not perfect, but good enough
	T res = RayTMin(origin);
	res *= (T(16) * (T(2) - cos_theta));
	//res = Max(res, NumericLimits<T>::min() * T(64));
	return res;
}

template <typename T : __BuiltinArithmeticType, int N>
T RayTMin(const in Ray<T, N> ray, const in Vector<T, N> normal)
{
	const T cos_theta = Abs(Dot(ray.direction, normal));
	return RayTMin(ray.origin, cos_theta);
}

#if SHADER_RAY_QUERY_AVAILABLE

template <uint Flags>
extension RayQuery<Flags>
{
	[mutating]
	func getRayInstanceCustomIndex<bool Committed>() -> int
	{
		if(Committed)	return CommittedRayInstanceCustomIndex();
		else			return CandidateRayInstanceCustomIndex();
	}
	[mutating]
	func getRayInstanceId<bool Committed>() -> int
	{
		if(Committed)	return CommittedRayInstanceId();
		else			return CandidateRayInstanceId();
	}
	[mutating]
	func getRayInstanceShaderBindingTableRecordOffset<bool Committed>() -> uint
	{
		if(Committed)	return CommittedRayInstanceShaderBindingTableRecordOffset();
		else			return CandidateRayInstanceShaderBindingTableRecordOffset();
	}
	[mutating]
	func getRayGeometryIndex<bool Committed>() -> int
	{
		if(Committed)	return CommittedRayGeometryIndex();
		else			return CandidateRayGeometryIndex();
	}
	[mutating]
	func getRayPrimitiveIndex<bool Committed>() -> int
	{
		if(Committed)	return CommittedRayPrimitiveIndex();
		else			return CandidateRayPrimitiveIndex();
	}
	[mutating]
	func getRayBarycentrics<bool Committed>() -> float2
	{
		if(Committed)	return CommittedRayBarycentrics();
		else			return CandidateRayBarycentrics();
	}
	[mutating]
	func getRayFrontFace<bool Committed>() -> bool
	{
		if(Committed)	return CommittedRayFrontFace();
		else			return CandidateRayFrontFace();
	}
	[mutating]
	func getRayObjectRayDirection<bool Committed>() -> float3
	{
		if(Committed)	return CommittedRayObjectRayDirection();
		else			return CandidateRayObjectRayDirection();
	}
	[mutating]
	func getRayObjectRayOrigin<bool Committed>() -> float3
	{
		if(Committed)	return CommittedRayObjectRayOrigin();
		else			return CandidateRayObjectRayOrigin();
	}
	[mutating]
	func getRayObjectToWorld<bool Committed>() -> float4x3
	{
		if(Committed)	return CommittedRayObjectToWorld();
		else			return CandidateRayObjectToWorld();
	}
	[mutating]
	func getRayWorldToObject<bool Committed>() -> float4x3
	{
		if(Committed)	return CommittedRayWorldToObject();
		else			return CandidateRayWorldToObject();
	}
	#if SHADER_RAY_TRACING_POSITION_FETCH_AVAILABLE
	[mutating]
	func getGetIntersectionTriangleVertexPositions<bool Committed>() -> vec3[3]
	{
		if(Committed)	return CommittedGetIntersectionTriangleVertexPositions();
		else			return CandidateGetIntersectionTriangleVertexPositions();
	}
#endif
	[mutating]
	func getObjectToWorld3x4<bool Committed>() -> float3x4
	{
		if(Committed)	return CommittedObjectToWorld3x4();
		else			return CandidateObjectToWorld3x4();
	}
	[mutating]
	func getObjectToWorld4x3<bool Committed>() -> float4x3
	{
		if(Committed)	return CommittedObjectToWorld4x3();
		else			return CandidateObjectToWorld4x3();
	}
	[mutating]
	func getWorldToObject3x4<bool Committed>() -> float3x4
	{
		if(Committed)	return CommittedWorldToObject3x4();
		else			return CandidateWorldToObject3x4();
	}
	[mutating]
	func getWorldToObject4x3<bool Committed>() -> float4x3
	{
		if(Committed)	return CommittedWorldToObject4x3();
		else			return CandidateWorldToObject4x3();
	}
	[mutating]
	func getInstanceIndex<bool Committed>() -> uint
	{
		if(Committed)	return CommittedInstanceIndex();
		else			return CandidateInstanceIndex();
	}
	[mutating]
	func getInstanceID<bool Committed>() -> uint
	{
		if(Committed)	return CommittedInstanceID();
		else			return CandidateInstanceID();
	}
	[mutating]
	func getPrimitiveIndex<bool Committed>() -> uint
	{
		if(Committed)	return CommittedPrimitiveIndex();
		else			return CandidatePrimitiveIndex();
	}
	[mutating]
	func getGeometryIndex<bool Committed>() -> uint
	{
		if(Committed)	return CommittedGeometryIndex();
		else			return CandidateGeometryIndex();
	}
	[mutating]
	func getInstanceContributionToHitGroupIndex<bool Committed>() -> uint
	{
		if(Committed)	return CommittedInstanceContributionToHitGroupIndex();
		else			return CandidateInstanceContributionToHitGroupIndex();
	}
	[mutating]
	func getObjectRayOrigin<bool Committed>() -> float3
	{
		if(Committed)	return CommittedObjectRayOrigin();
		else			return CandidateObjectRayOrigin();
	}
	[mutating]
	func getObjectRayDirection<bool Committed>() -> float3
	{
		if(Committed)	return CommittedObjectRayDirection();
		else			return CandidateObjectRayDirection();
	}
	[mutating]
	func getTriangleFrontFace<bool Committed>() -> bool
	{
		if(Committed)	return CommittedTriangleFrontFace();
		else			return CandidateTriangleFrontFace();
	}
	[mutating]
	func getTriangleBarycentrics<bool Committed>() -> float2
	{
		if (Committed)	return CommittedTriangleBarycentrics();
		else			return CandidateTriangleBarycentrics();
	}
};

#if SHADER_RAY_TRACING_POSITION_FETCH_AVAILABLE

#endif

#endif