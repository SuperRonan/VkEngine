
#define BIND_SCENE 1
#define BIND_SCENE_TLAS 1
#define BIND_ALL_SCENE_COMPONENTS 1
#define BIND_RENDERER_SET 1

#ifndef FORCE_COSINE_SAMPLING 
#define FORCE_COSINE_SAMPLING 0
#endif

#define DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL 1
#define DEBUG_ENABLE_GLOBAL_PRINTER 1
#define I_WANT_TO_DEBUG 1


#include "../common.slang"

#define SHADING_SHADOW_METHOD SHADING_SHADOW_RAY_TRACE
#include "../shading.slang"

#include <ShaderLib/Maths/transforms.slang>

#include <ShaderLib/Color.slang>


#include <ShaderLib/Debug/DebugBuffers.slang>

layout(SHADER_DESCRIPTOR_BINDING + 2, rgba32f) uniform restrict RWTexture2D<float4> Target;

struct PushConstant
{
	uint value;
};

[vk::push_constant]
uniform PushConstant _pc;

#define MAX_DEPTH 2
#define LIGHT_SAMPLES 16

#define LOCAL_SIZE_X 16
#define LOCAL_SIZE_Y 16
#define LOCAL_SIZE_Z 1

__generic <T : __BuiltinFloatingPointType, int N>
Matrix<T, N , 2> ddxy(const in Vector<T, N> v)
{
	return MakeFromCols(ddx(v), ddy(v));
}

vec2 DifferentiateSurfaceIntersectionT(const in vec3 ray_dir, const in vec3 normal, float t, const in RayDiff3f ray_diffs)
{
	const float abs_cos_theta = Abs(Dot(normal, ray_dir));
	const vec2 res = (Transpose(ray_diffs.origin_jacobian + t * ray_diffs.direction_jacobian) * normal) / abs_cos_theta;
	return res;
}

struct TraversingRay
{
	Ray3f ray;
	RayDiff3f ray_diffs;
	RayRangef range;
	vec3 throughput;

	__init(const in CameraSample sample)
	{
		ray = sample.ray.ray;
		ray_diffs = sample.ray.differentials;
		range = sample.ray.range;
		throughput = sample.importance / sample.pdf;
	}
}

[shader("compute")]
[numthreads(LOCAL_SIZE_X, LOCAL_SIZE_Y, LOCAL_SIZE_Z)]
void main(const uvec3 GlobalInvocationID : SV_DispatchThreadID)
{
	const uvec2 pixel = GlobalInvocationID.xy;
	const uvec2 dims = TextureSize(Target);
	const vec2 oo_dims = rcp(vec2(dims));
	if(all(pixel < dims))
	{

		BoundScene scene;

		RNG_t rng = RNG_t(Hash(pixel) ^ Hash(renderer_ubo.frame_idx));

		vec3 res = vec3(0);
		
		PerspectiveCamera cam = PerspectiveCamera(renderer_ubo.camera, oo_dims);
		
		const vec2 uv = (vec2(pixel) + 0.5f) * oo_dims;
		const vec2 cp = UVToClipSpace(uv);
		
		Matrix2f cp_jacobian = DiagonalMatrixV(oo_dims * 2);
		CameraSample cam_sample = cam.sampleWe(rng, cp, cp_jacobian);
		TraversingRay tray = TraversingRay(cam_sample);
		
		uint num_vertices = 1;
		const uint max_vertices = MAX_DEPTH + 1;

		while(num_vertices < max_vertices)
		{
			const uint RayFlags = RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
			RayQuery<RayFlags> rq = {};
			rq.TraceRayInline(SceneTLAS, 0, ~0, RayDesc(tray.ray, tray.range));
			scene.proceedRayQuery(rq);
			
			if(rq.CommittedStatus() == COMMITTED_NOTHING)
			{
				break;
			}
			else if (rq.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
			{
				const uint object_index = rq.getRayInstanceCustomIndex<true>();
				const uint primitive_index = rq.getPrimitiveIndex<true>();
				const Scene::ObjectReference object_ref = SceneObjectsTable[object_index];
				BoundScene::MeshReference mesh = BoundScene::MeshReference(object_ref.mesh_id);
				
				SurfaceShadingInfo ssi = scene.reconstructTriangleSurfaceShadingInfo<true>(rq);
				ssi.out_direction = -tray.ray.direction;
	
				const float t = rq.CommittedRayT();
				const vec2 t_diffs = DifferentiateSurfaceIntersectionT(tray.ray.direction, ssi.geometry_normal, t, tray.ray_diffs);
	
				RayDiff3f new_ray_diffs = tray.ray_diffs.transfer(t, t_diffs, tray.ray.direction);
	
				Matrix3f object_to_world = ExtractQBlock(rq.getObjectToWorld3x4<true>());
				Matrix3f world_to_object = ExtractQBlock(rq.getWorldToObject3x4<true>());
				Matrix3x2f new_origin_jacobian_obj = world_to_object * new_ray_diffs.origin_jacobian;
	
				uvec3 triangle_indices = mesh.getPrimitiveIndices<3>(rq.getPrimitiveIndex<true>());
				let surfaceDifferentials = mesh.getTriangleDifferentials(DirectionMatrix(object_to_world), triangle_indices, ssi.interpolated_normal, new_origin_jacobian_obj);
				
				Matrix2f Juv = surfaceDifferentials.uv_jacobians;
				RayDiffTextureSampling sampling = RayDiffTextureSampling(Juv);
				PBMaterial material = scene.readMaterial(object_ref.material_id, ssi.uv, true, sampling);
	
				DirectLightSurfaceShadingEachLight shader;
				shader.scene = scene;
				shader.sample_per_light = 1;
				res += tray.throughput * shader.shade(ssi, material, rng);

				
				++num_vertices;
				let bsdf_sample = material.sampleBSDF(
					ssi.geometry_normal,
					ssi.shading_normal,
					surfaceDifferentials.normal_jacobian,
					ssi.out_direction,
					-new_ray_diffs.direction_jacobian,
					rng);
				if(bsdf_sample.pdf <= 0.0f)
				{
					break;
				}
				tray.ray = Ray3f(ssi.position, bsdf_sample.direction);
				tray.ray_diffs.origin_jacobian = new_ray_diffs.origin_jacobian;
				tray.ray_diffs.direction_jacobian = bsdf_sample.direction_jacobian;
				tray.range.resetRange();
				tray.range.min = RayTMin(tray.ray, ssi.geometry_normal);
				const float cos_theta = Abs(Dot(ssi.shading_normal, bsdf_sample.direction));
				tray.throughput *= bsdf_sample.bsdf * cos_theta / bsdf_sample.pdf;
			}
		}

		Target.Store(pixel, vec4(res, 1));
	}
}

