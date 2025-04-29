#pragma once

#include "../common.slang"

#include <ShaderLib/Rendering/Scene/Scene.slang>

struct UBO : RendererUBO
{

};

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
		throughput = sample.intensity / sample.pdf;
	}

	__init(const in CameraSample sample)
	{
		ray = sample.ray.ray;
		ray_diffs = sample.ray.differentials;
		range = sample.ray.range;
		throughput = sample.importance / sample.pdf;
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

#endif

interface IRayTracer
{
	Hit traceRay<int flags>(const in RayTraceInfo info);
};

// struct RayTracer : IRayTracer
// {
// 	Hit traceRay(const in Ray3f ray, const in RayRangef range = {}, const in RayDiff3f diffs = {})
// 	{

// 	}
// };

#if BIND_SCENE && defined(SCENE_MESH_ACCESS) && defined(SCENE_MATERIAL_ACCESS) && BIND_SCENE_TLAS && defined(SCENE_OBJECTS_ACCESS) && defined(SCENE_XFORM_ACCESS)

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

	__init(BoundScene scene)
	{
		this.scene = scene;
	}
}

#endif

#if BIND_RENDERER_SET

layout(RENDERER_BINDING + 0) uniform ConstantBuffer<RendererUBO> rt_ubo;

#endif