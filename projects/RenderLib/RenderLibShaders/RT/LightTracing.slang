
#define BIND_SCENE 1
#define BIND_SCENE_TLAS 1
#define BIND_ALL_SCENE_COMPONENTS 1
#define BIND_RENDERER_SET 1

#ifndef FORCE_COSINE_SAMPLING 
#define FORCE_COSINE_SAMPLING 0
#endif

#define I_WANT_TO_DEBUG 1
#define DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL I_WANT_TO_DEBUG
#define DEBUG_ENABLE_GLOBAL_PRINTER I_WANT_TO_DEBUG

//#define FORCE_CAMERA_TYPE CAMERA_TYPE_ORTHO
#define FORCE_CAMERA_TYPE CAMERA_TYPE_PERSPECTIVE

//#define DEFAULT_HASH_POLICY HASH_POLICY_SLOW

#include "Common.slang"

#define SHADING_SHADOW_METHOD SHADING_SHADOW_RAY_TRACE
#include "../shading.slang"

#include <ShaderLib/Maths/transforms.slang>

#include <ShaderLib/Color.slang>


#include <ShaderLib/Debug/DebugBuffers.slang>

layout(SHADER_DESCRIPTOR_BINDING + 2) restrict RWStructuredBuffer<Vector3<LightBufferAccumFloat>, Std430DataLayout> Target;

#define LOCAL_SIZE_X 256
#define LOCAL_SIZE_Y 1
#define LOCAL_SIZE_Z 1

struct PushConstant
{
	vec2 oo_dims;
	vec2 dims;
	uint2 udims;
	float dispatched_threads;
};

[vk::push_constant]
uniform PushConstant _pc;

//[require(GL_EXT_shader_atomic_float)]
void ConnectToCameraIFP<RNG : IRNG>(
	const in BoundScene scene,
	const in ICamera camera,
	const in Hit hit,
	const in IMaterial material,
	vec3 incoming_throughput,
	inout RNG rng)
{
	Camera::ConnectionSample connection = camera.sampleWi(rng, hit.surface_shading_info.position);
	if(connection.pdf > 0 && NonZero(connection.intensity))
	{
		const vec3 wi = connection.direction;
		const vec3 wo = hit.surface_shading_info.out_direction;
		vec3 bsdf = material.bsdf<BSDF_ADJOINT_BIT>(
			hit.surface_shading_info.geometry_normal, 
			hit.surface_shading_info.shading_normal, 
			wo, wi
		);
		if(NonZero(bsdf))
		{
			RayRangef range = {};
			let _ray = Ray3f(hit.surface_shading_info.position, connection.direction);
			range.min = RayTMin(_ray, hit.surface_shading_info.geometry_normal);
			range.max = connection.distance;
			let ray = RayDesc(_ray, range);
			float v = scene.testVisibility(ray);
			const vec3 ns = hit.surface_shading_info.shading_normal;
			const vec3 ng = hit.surface_shading_info.geometry_normal;
			const float cos_theta_i_s = Dot(ns, wi);
			const float cos_theta_i_g = Dot(ng, wi);
			const float cos_theta_o_g = Dot(ng, wo);
			const float cos_theta_o_s = Dot(ns, wo);
			// correction of shading normals 
			const float correction = Abs((cos_theta_o_s * cos_theta_i_g) / (cos_theta_o_g * cos_theta_i_s));
			vec3 contrib = incoming_throughput * bsdf * connection.intensity * (v * Abs(cos_theta_i_s) * correction / connection.pdf);
			if(NonZero(contrib))
			{
				AddLightTracerSample(Target, connection.uv, _pc.dims, _pc.udims, contrib);
			}
		}
	}
}






void TracePath<RNG : IRNG>(inout RNG rng, const in ICamera camera, const in IRayTracer ray_tracer, const in BoundScene scene, TraversingRay tray)
{
	uint num_vertices = 1;
	const uint max_vertices = MAX_DEPTH + 1;
	while(num_vertices < max_vertices)
	{
		RayTraceInfo trace_info = {};
		trace_info.ray = tray.ray;
		trace_info.range = tray.range;
		Hit hit = ray_tracer.traceRay<RAY_FLAG_NONE>(trace_info);
		if(!hit.hasValue())
		{
			break;
		}
		++num_vertices;

		DefaultTextureSampling sampling = {};
		let material = scene.readMaterial(hit.material_id, hit.surface_shading_info.uv, true, sampling);

		ConnectToCameraIFP(scene, camera, hit, material, tray.throughput, rng);
		
		let bsdf_sample = material.sampleBSDF<BSDF_ADJOINT_BIT>(
			hit.surface_shading_info.geometry_normal,
			hit.surface_shading_info.shading_normal,
			hit.surface_diffs.normal_jacobian,
			hit.surface_shading_info.out_direction,
			-tray.ray_diffs.direction_jacobian,
			rng);
		if(bsdf_sample.pdf <= 0.0f)
		{
			break;
		}

		tray.ray = Ray3f(hit.surface_shading_info.position, bsdf_sample.direction);
		tray.range.resetRange();
		tray.range.min = RayTMin(tray.ray, hit.surface_shading_info.geometry_normal);
		const float cos_theta_i_s = AbsDot(hit.surface_shading_info.shading_normal, bsdf_sample.direction);
		const float cos_theta_i_g = AbsDot(hit.surface_shading_info.geometry_normal, bsdf_sample.direction);
		const float cos_theta_o_s = AbsDot(hit.surface_shading_info.shading_normal, hit.surface_shading_info.out_direction);
		const float cos_theta_o_g = AbsDot(hit.surface_shading_info.geometry_normal, hit.surface_shading_info.out_direction);
		const float correction = (cos_theta_o_s * cos_theta_i_g) / (cos_theta_o_g * cos_theta_i_s);
		tray.throughput *= bsdf_sample.bsdf * (cos_theta_i_s / bsdf_sample.pdf * correction);
	}
}

public property uint3 gl_NumWorkGroups {
	[require(glsl_spirv, GLSL_430_SPIRV_1_0_compute)]
	get 
	{
		__target_switch
		{
		case glsl:
			__intrinsic_asm "(gl_NumWorkGroups)";
		case spirv:
			return spirv_asm {
					result:$$uint3 = OpLoad builtin(NumWorkgroups:uint3);
				};
		}
	}
}


[shader("compute")]
[numthreads(LOCAL_SIZE_X, LOCAL_SIZE_Y, LOCAL_SIZE_Z)]
void main(
	const uvec3 GlobalInvocationID : SV_DispatchThreadID,
	const uvec3 WorkGroupID : SV_GroupID)
{
	const uint group_index = WorkGroupID.x;
	const uint thread_index = GlobalInvocationID.x;
	const float num_samples = float(gl_NumWorkGroups.x * LOCAL_SIZE_X);
	const vec2 dims = _pc.dims;

	const float sample_weight = Prod(dims) / num_samples;

	BoundScene scene;
	RayQuerier ray_querier = RayQuerier(scene);
	const vec2 oo_dims = _pc.oo_dims;
	ICamera cam = MakeCamera(renderer_ubo.camera);

	RNG_t rng = RNG_t(Hash(thread_index) ^ Hash(renderer_ubo.frame_idx));

	Light::EmissionSample sample = scene.sampleLe(rng);
	sample.intensity *= sample_weight;
	if(!NonZero(sample.intensity) || sample.pdf <= 0)
	{
		return;
	}
	TraversingRay tray = TraversingRay(sample);
	TracePath(rng, cam, ray_querier, scene, tray);

}

