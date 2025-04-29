
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
		vec3 bsdf = material.bsdf(
			hit.surface_shading_info.geometry_normal, 
			hit.surface_shading_info.shading_normal, 
			hit.surface_shading_info.out_direction,
			connection.ray.direction);
		if(NonZero(bsdf))
		{
			connection.range.min = RayTMin(connection.ray, hit.surface_shading_info.geometry_normal);
			let ray = RayDesc(connection.ray, connection.range);
			float v = scene.testVisibility(ray);
			const float cos_theta = Abs(Dot(hit.surface_shading_info.shading_normal, connection.ray.direction));
			vec3 contrib = incoming_throughput * bsdf * cos_theta * v * connection.importance / connection.pdf;
			//contrib *= Prod(_pc.dims);
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



#define MAX_DEPTH 5
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
		const float cos_theta = Abs(Dot(hit.surface_shading_info.shading_normal, bsdf_sample.direction));
		tray.throughput *= bsdf_sample.bsdf * cos_theta / bsdf_sample.pdf;
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
	const uint num_samples = WorkgroupSize().x * LOCAL_SIZE_X;


	
	BoundScene scene;
	RayQuerier ray_querier = RayQuerier(scene);
	const vec2 oo_dims = _pc.oo_dims;
	PerspectiveCamera cam = PerspectiveCamera(renderer_ubo.camera, oo_dims);

	RNG_t rng = RNG_t(Hash(thread_index) ^ Hash(renderer_ubo.frame_idx));

	
	const uint light_index = rng.generate<uint>(0, scene.getUBO().num_lights);
	const float pmf_ligh = rcp(scene.getUBO().num_lights);
	
	TraversingRay tray;
	ILight light;
	if(MakeLight(SceneLights[light_index], light))
	{
		Light::EmissionSample sample = light.sampleLe(rng);
		sample.pdf *= pmf_ligh;
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
		
	TracePath(rng, cam, ray_querier, scene, tray);

}

