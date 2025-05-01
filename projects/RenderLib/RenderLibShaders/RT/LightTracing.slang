
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

layout(SHADER_DESCRIPTOR_BINDING + 2) restrict RWStructuredBuffer<float3, Std430DataLayout> Target;

struct PushConstant
{
	vec2 oo_dims;
	vec2 dims;
	uint2 udims_minus_1;
	float dispatched_threads;
};

void InterlockedAddVec<T : IArithmeticAtomicable, int N>(Ptr<Vector<T, N>> dest, const in Vector<T, N> value)
{
	for(uint i = 0; i < N; ++i)
	{
		InterlockedAdd((*dest)[i], value[i]);
	}
}

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
	CameraConnection connection = camera.sampleWi(rng, hit.surface_shading_info.position);
	if(connection.pdf > 0 && NonZero(connection.importance))
	{
		const vec3 wi = connection.ray.direction;
		const vec3 wo = hit.surface_shading_info.out_direction;
		// swap wi and wo to evaluate the adjoint bsdf
		vec3 bsdf = material.bsdf(
			hit.surface_shading_info.geometry_normal, 
			hit.surface_shading_info.shading_normal, 
			wi, wo
		);
		if(NonZero(bsdf))
		{
			connection.range.min = RayTMin(connection.ray, hit.surface_shading_info.geometry_normal);
			let ray = RayDesc(connection.ray, connection.range);
			float v = scene.testVisibility(ray);
			const vec3 ns = hit.surface_shading_info.shading_normal;
			const vec3 ng = hit.surface_shading_info.geometry_normal;
			const float cos_theta_i_s = Dot(ns, wi);
			const float cos_theta_i_g = Dot(ng, wi);
			const float cos_theta_o_g = Dot(ng, wo);
			const float cos_theta_o_s = Dot(ns, wo);
			// correction of shading normals 
			const float correction = Abs((cos_theta_o_s * cos_theta_i_g) / (cos_theta_o_g * cos_theta_i_s));
			vec3 contrib = incoming_throughput * bsdf * connection.importance * (v * Abs(cos_theta_i_s) * correction / connection.pdf);
			if(NonZero(contrib))
			{
				vec2 fpixel = connection.uv * _pc.dims;
				uint2 pixel = uint2(fpixel);
				if(all(min(pixel, _pc.udims_minus_1) == pixel))
				{
					const uint address = pixel.x + pixel.y * (_pc.udims_minus_1.x + 1);
					for(uint i = 0; i < 3; ++i)
					{
						InterlockedAdd(Target[address][i], contrib[i]);
					}
				}
			}
		}
	}
}




#define LIGHT_SAMPLES 16

#define LOCAL_SIZE_X 256
#define LOCAL_SIZE_Y 1
#define LOCAL_SIZE_Z 1

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
		
		let bsdf_sample = material.sampleBSDF(
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

	
	const uint light_index = rng.generate<uint>(0, scene.getUBO().num_lights);
	const float pmf_ligh = rcp(scene.getUBO().num_lights);

	_g_debug_signal = thread_index == 0;
	if(_g_debug_signal)
	{
		_g_debug_printer = DebugPrinter(vec3(0), DEBUG_PIXEL_SPACE_BIT, FONT_SIZE_NORMAL + 0.5f);
	}
	
	TraversingRay tray;
	ILight light;
	if(MakeLight(SceneLights[light_index], light))
	{
		Light::EmissionSample sample = light.sampleLe(rng);
		sample.pdf *= pmf_ligh;
		sample.intensity *= sample_weight;
		if(!NonZero(sample.intensity) || sample.pdf <= 0)
		{
			return;
		}
		tray = TraversingRay(sample);
	}
	else
	{
		return;
	}

	if(_g_debug_signal)
	{
		//_g_debug_printer.print(tray.throughput);
	}

	TracePath(rng, cam, ray_querier, scene, tray);

}

