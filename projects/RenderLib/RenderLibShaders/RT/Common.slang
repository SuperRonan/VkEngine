#pragma once

#include "../common.slang"

#include <ShaderLib/Rendering/Scene/Scene.slang>

struct RT_UBO : RendererUBO
{
	uint max_depth;
	uint flags;
};

typealias LightBufferAccumFloat = float;

__generic<Float : __BuiltinFloatingPointType, int N, Layout : IBufferDataLayout>
bool AddLightTracerSample(RWStructuredBuffer<Vector<LightBufferAccumFloat, N>, Layout> accum_buffer, vec2 uv, vec2 fdims, uvec2 udims, Vector<Float, N> value)
{
	vec2 fpixel = uv * fdims;
	uint2 pixel = uint2(fpixel);
	bool res = false;
	if(all(pixel < udims))
	{
		const uint address = pixel.x + pixel.y * udims.x;
		for(uint i = 0; i < N; ++i)
		{
			InterlockedAdd(accum_buffer[address][i], ConvertBuiltInTo<LightBufferAccumFloat>(value[i]));
		}
		res = true;
	}
	return res;
}

struct TraversingRay
{
	Ray3f ray;
	RayDiff3f ray_diffs;
	RayRangef range;
	vec3 throughput;

	__init(const in Light::EmissionSample sample)
	{
		ray = sample.ray;
		ray_diffs = {};
		range = sample.range;
		throughput = sample.intensity / (sample.pdf * sample.position_pdf);
	}

	__init(const in Camera::EmissionSample sample)
	{
		ray = sample.ray.ray;
		ray_diffs = sample.ray.differentials;
		range = sample.ray.range;
		throughput = sample.intensity / (sample.pdf * sample.position_pdf);
	}
};


struct Hit
{
	[Flags]
	enum Flags
	{
		None = 0,
		Triangle = (1 << 0),
		Procedural = (1 << 1),
		FontFacing = (1 << 4),
		BackFacing = (1 << 5),
	}
	Flags flags = Flags::None;
	
	float t = -1.0f;

	uint custom_index = {};
	uint instance_index = {};
	uint primitive_index = {};
	uint geometry_index = {};
	uint sbt_offset = {};
	uint material_id = {};
	
	Matrix3x4f world_to_object = {};
	Matrix3x4f object_to_world = {};

	SurfaceShadingInfo surface_shading_info = {};
	SurfaceGeometryDifferentials2 surface_diffs = {};
	

	__init() {}

	bool hasValue()
	{
		return flags != Flags::None;
	}
};

struct TracePayload
{
	uint flags;
	uint custom_index;
	uint primitive_index;
	u32 uv_pack;
};

struct RayTraceInfo
{
	Ray3f ray;
	
	RayRangef range = {};
	RayDiff3f diffs = {};
	uint ray_flags = 0;
	uint mask = ~0;
}

#if BIND_SCENE

void computeHitMeshDifferentials(const in RayTraceInfo info, inout Hit res)
{
	const Scene::ObjectReference object_ref = SceneObjectsTable[res.custom_index];
	const BoundScene::MeshReference mesh = BoundScene::MeshReference(object_ref.mesh_id);
	const uvec3 triangle_indices = mesh.getPrimitiveIndices<3>(res.primitive_index);
	const vec2 t_diffs = DifferentiateSurfaceIntersectionT(info.ray.direction, res.surface_shading_info.geometry_normal, res.t, info.diffs);

	let Jposition = info.diffs.transfer(res.t, t_diffs, info.ray.direction).origin_jacobian;

	res.surface_diffs = mesh.getTriangleDifferentials(
		DirectionMatrix(res.object_to_world), 
		triangle_indices, 
		res.surface_shading_info.interpolated_normal, 
		ExtractQBlock(res.world_to_object) * Jposition
	);
	res.surface_diffs.position_jacobian = Jposition;
}

interface IRayTracer
{
	Hit traceRay<int flags>(const in RayTraceInfo info);

	float testVisibility(const in RayDesc ray);
};

struct RayTracer : IRayTracer
{
	BoundScene scene;

	__init(BoundScene scene)
	{
		this.scene = scene;
	}

	Hit traceRay<int flags>(const in RayTraceInfo info)
	{
		Hit res;
		res.surface_shading_info.out_direction = -info.ray.direction;
		res.surface_diffs.position_jacobian = info.diffs.origin_jacobian;
		res.surface_diffs.normal_jacobian = info.diffs.direction_jacobian;

		TracePayload payload;
		payload.flags = 0;

		uint ray_offset = 0;
		uint ray_multiplier = 0;
		uint miss_index = 0;

		TraceRay(
			SceneTLAS,
			uint(flags) | info.ray_flags,
			info.mask,
			ray_offset, ray_multiplier, miss_index,
			RayDesc(info.ray, info.range),
			payload
		);
		res.flags = Hit::Flags(payload.flags);
		if(res.hasValue())
		{
			res.custom_index = payload.custom_index;
			res.primitive_index = payload.primitive_index;
			vec2 bary = unpackUnorm2x16ToFloat(payload.uv_pack);
			const Scene::ObjectReference object_ref = SceneObjectsTable[payload.custom_index];
			res.material_id = object_ref.material_id;

			res.object_to_world = SceneXForms[object_ref.xform_id];
			res.world_to_object = InverseAffineTransform(res.object_to_world);
			res.surface_shading_info = scene.reconstructTriangleSurfaceShadingInfo(
				res.custom_index, res.primitive_index, bary,
				res.object_to_world
			);
			res.surface_shading_info.out_direction = -info.ray.direction;
			res.t = distance(info.ray.origin, res.surface_shading_info.position);
			computeHitMeshDifferentials(info, res);
		}
		return res;
	}

	float testVisibility(const in RayDesc ray)
	{
		float res;
		
		uint ray_offset = 1;
		uint ray_multiplier = 0;
		uint miss_index = 1;

		TraceRay(
			SceneTLAS,
			RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
			~0,
			ray_offset, ray_multiplier, miss_index,
			ray,
			res
		);
		return res;
	}
};

#if BIND_SCENE && defined(SCENE_MESH_ACCESS) && defined(SCENE_MATERIAL_ACCESS) && BIND_SCENE_TLAS && defined(SCENE_OBJECTS_ACCESS) && defined(SCENE_XFORM_ACCESS)

#if SHADER_RAY_QUERY_AVAILABLE
struct RayQuerier : IRayTracer
{
	BoundScene scene;

	Hit traceRay<int static_flags>(const in RayTraceInfo info)
	{
		Hit res = {};
		res.surface_shading_info.out_direction = -info.ray.direction;

		const uint static_ray_flags = RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | uint(static_flags);
		RayQuery<static_ray_flags> rq = {};
		rq.TraceRayInline(SceneTLAS, info.ray_flags, info.mask, RayDesc(info.ray, info.range));
		scene.proceedRayQuery(rq);
		if(rq.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
		{
			res.flags = res.flags | Hit::Flags::Triangle;
			res.flags = res.flags | (rq.CommittedRayFrontFace() ? Hit::Flags::FontFacing : Hit::Flags::BackFacing);
			
			res.t = rq.CommittedRayT();

			res.custom_index = rq.CommittedRayInstanceCustomIndex();
			res.instance_index = rq.CommittedInstanceIndex();
			res.primitive_index = rq.CommittedPrimitiveIndex();
			res.geometry_index = rq.CommittedGeometryIndex();
			res.sbt_offset = rq.CommittedRayInstanceShaderBindingTableRecordOffset();

			const Scene::ObjectReference object_ref = SceneObjectsTable[res.custom_index];
			res.material_id = object_ref.material_id;
			
			res.world_to_object = rq.CommittedWorldToObject3x4();
			res.object_to_world = rq.CommittedObjectToWorld3x4();

			res.surface_shading_info = scene.reconstructTriangleSurfaceShadingInfo<true>(rq);
			res.surface_shading_info.out_direction = -info.ray.direction;
			
			computeHitMeshDifferentials(info, res);
		}
		return res;
	}

	float testVisibility(const in RayDesc ray)
	{
		return scene.testVisibility(ray);
	}

	__init(BoundScene scene)
	{
		this.scene = scene;
	}
}
#endif

#endif

#endif

#if BIND_RENDERER_SET

layout(RENDERER_BINDING + 0) uniform ConstantBuffer<RT_UBO> rt_ubo;

#endif

#if BIND_RENDERER_SET
uint GetMaxDepth()
{
#ifdef MAX_DEPTH
	return MAX_DEPTH;
#elif BIND_RENDERER_SET
	return rt_ubo.max_depth;
#else
	return 1;
#endif
}
#endif
