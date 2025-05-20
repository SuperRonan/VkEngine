
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

//#define DEFAULT_HASH_POLICY HASH_POLICY_SLOW

#include "Common.slang"

#define SHADING_SHADOW_METHOD SHADING_SHADOW_RAY_TRACE
#include "../shading.slang"

#include <ShaderLib/Maths/transforms.slang>

#include <ShaderLib/Color.slang>


#include <ShaderLib/Debug/DebugBuffers.slang>

layout(SHADER_DESCRIPTOR_BINDING + 2) restrict RWStructuredBuffer<Vector3<LightBufferAccumFloat>, Std430DataLayout> Target;

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
	const in IRayTracer ray_tracer,
	RayDesc ray,
	vec2 uv,
	vec3 contrib,
	inout RNG rng
) {
	let v = ray_tracer.testVisibility(ray);
	if(NonZero(v))
	{
		AddLightTracerSample(Target, uv, _pc.dims, _pc.udims, contrib * v);
	}
}


void ConnectToCameraIFP<RNG : IRNG>(
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	const in Hit hit,
	const in IMaterial material,
	vec3 incoming_throughput,
	inout RNG rng)
{
	ICamera camera = MakeCamera(rt_ubo.camera);
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
			const vec3 ns = hit.surface_shading_info.shading_normal;
			const vec3 ng = hit.surface_shading_info.geometry_normal;
			const float cos_theta_i_s = Dot(ns, wi);
			const float cos_theta_i_g = Dot(ng, wi);
			const float cos_theta_o_g = Dot(ng, wo);
			const float cos_theta_o_s = Dot(ns, wo);
			// correction of shading normals 
			const float correction = Abs((cos_theta_o_s * cos_theta_i_g) / (cos_theta_o_g * cos_theta_i_s));
			vec3 contrib = incoming_throughput * bsdf * connection.intensity * (Abs(cos_theta_i_s) * correction / connection.pdf);
			if(NonZero(contrib))
			{
				let ray = Ray3f(hit.surface_shading_info.position, connection.direction);
				RayRangef range = {};
				range.min = RayTMin(ray, hit.surface_shading_info.geometry_normal);
				range.max = connection.distance;
				ConnectToCameraIFP(scene, ray_tracer, RayDesc(ray, range), connection.uv, contrib, rng);
			}
		}
	}
}

void ConnectLightToCameraIFP<RNG : IRNG>(
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	const in Light::EmissionSample sample,
	const in vec3 beta,
	inout RNG rng
) {
	ICamera camera = MakeCamera(rt_ubo.camera);
	vec3 ref_pos = sample.ray.origin;
	uint flags = 0;
	const bool at_infinity = sample.flags & Light::EmissionSample::Flags::AtInfinity;
	if(at_infinity)
	{
		ref_pos = sample.ray.direction;
		flags |= CAMERA_SAMPLE_POSITION_AT_INFINITY;
	}
	Camera::ConnectionSample connection = camera.sampleWi(rng, ref_pos, flags);
	if(connection.pdf > 0 && NonZero(connection.intensity))
	{
		// TODO implement Light::Le
		// For now the sample intensity will do fine
		vec3 Le = sample.intensity / sample.position_pdf;
		
		vec3 contrib = beta * Le * connection.intensity / connection.pdf;
		if(NonZero(contrib))
		{
			Ray3f ray;
			RayRangef range = {};
			if(at_infinity)
			{
				ray = Ray3f(connection.position, -connection.direction);
			}
			else
			{
				// TODO tmin for surface lighs
				ray = Ray3f(sample.ray.origin, connection.direction);
				range.max = connection.distance;
			}
			ConnectToCameraIFP(scene, ray_tracer, RayDesc(ray, range), connection.uv, contrib, rng);
		}
	}
}




void TracePath<RNG : IRNG>(inout RNG rng, const in IRayTracer ray_tracer, const in BoundScene scene, TraversingRay tray)
{
	uint num_vertices = 1;
	const uint max_vertices = GetMaxDepth() + 1;
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
		
		//if(num_vertices == 2)
		{
			//tray.throughput *= rcp(AbsDot(tray.ray.direction, hit.surface_shading_info.geometry_normal));
		}

		DefaultTextureSampling sampling = {};
		let material = scene.readMaterial(hit.material_id, hit.surface_shading_info.uv, true, sampling);

		{
			let kappa = material.getKappa(hit.surface_shading_info.geometry_normal, -tray.ray.direction);
			if(kappa.hasValue)
			{
				tray.throughput *= exp2(-hit.t * kappa.value);
			}
		}

		ConnectToCameraIFP(scene, ray_tracer, hit, material, tray.throughput, rng);
		
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

void CommonMain(
	uint index, float weight,
	const in IRayTracer ray_tracer,
	BoundScene scene,

) {
	uint seed = Hash(index);
	seed = seed ^ Hash(renderer_ubo.frame_idx);
	//seed = Hash(renderer_ubo.frame_idx);
	RNG_t rng = RNG_t(seed);

	Light::EmissionSample sample = scene.sampleLe(rng);
	sample.intensity *= weight;
	if(!NonZero(sample.intensity) || sample.pdf <= 0)
	{
		return;
	}

	ConnectLightToCameraIFP(scene, ray_tracer, sample, 1, rng);

	TraversingRay tray = TraversingRay(sample);
	TracePath(rng, ray_tracer, scene, tray);
}

#if SHADER_SEMANTIC_COMPUTE
#define LOCAL_SIZE_X 256
#define LOCAL_SIZE_Y 1
#define LOCAL_SIZE_Z 1
[shader("compute")]
[numthreads(LOCAL_SIZE_X, LOCAL_SIZE_Y, LOCAL_SIZE_Z)]
void main(
	const uvec3 GlobalInvocationID : SV_DispatchThreadID,
	const uvec3 WorkGroupID : SV_GroupID
){
	const uint group_index = WorkGroupID.x;
	const uint thread_index = GlobalInvocationID.x;
	const float num_samples = float(gl_NumWorkGroups.x * LOCAL_SIZE_X);
	const vec2 dims = _pc.dims;

	const float sample_weight = Prod(dims) / num_samples;

	BoundScene scene;
	RayQuerier ray_querier = RayQuerier(scene);
	
	CommonMain(
		thread_index, sample_weight,
		ray_querier,
		scene
	);
}
#endif

#if SHADER_SEMANTIC_RAYGEN
[shader("raygen")]
void main(){
	const uint thread_index = DispatchRaysIndex().x;
	const float num_samples = DispatchRaysDimensions().x;
	const vec2 dims = _pc.dims;

	const float sample_weight = Prod(dims) / num_samples;

	BoundScene scene;
	RayTracer ray_querier = RayTracer(scene);
	
	CommonMain(
		thread_index, sample_weight,
		ray_querier,
		scene
	);
}
#endif