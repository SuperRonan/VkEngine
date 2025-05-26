
#define BIND_SCENE 1
#define BIND_SCENE_TLAS 1
#define BIND_ALL_SCENE_COMPONENTS 1
#define BIND_RENDERER_SET 1

#ifndef FORCE_COSINE_SAMPLING 
#define FORCE_COSINE_SAMPLING 0
#endif

#define I_WANT_TO_DEBUG 1
#define DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL 1
#define DEBUG_ENABLE_GLOBAL_PRINTER 1

#include "Common.slang"

#define SHADING_SHADOW_METHOD SHADING_SHADOW_RAY_TRACE
#include "../shading.slang"

#include <ShaderLib/Maths/transforms.slang>

#include <ShaderLib/Color.slang>


#include <ShaderLib/Debug/DebugBuffers.slang>

layout(SHADER_DESCRIPTOR_BINDING + 2, TARGET_IMAGE_FORMAT) uniform restrict RWTexture2D<float4> Target;

struct PushConstant
{
	uint value;
};

[vk::push_constant]
uniform PushConstant _pc;

#define LIGHT_SAMPLES 1
#define PATH_PER_PIXEL 1
struct DirectLightSurfaceShading<RayTracer : IRayTracer>
{
	BoundScene scene;
	RayTracer ray_tracer;
	uint samples = 1;

	vec3 shade <RNG : IRNG>(
		const in LiSampler sampler,
		const in SurfaceGeometryInfo sgi,
		const in IMaterial material,
		const in vec3 beta,
		inout RNG rng
	)
	{
		vec3 res = vec3(0);
		for(uint i = 0; i < samples; ++i)
		{
			let sample = sampler.sampleLi(rng, sgi, material, beta);
			if(sample.pdf > 0)
			{
				const vec3 bsdf = material.bsdf<BSDF_FORWARD_BIT>(sgi.geometry_normal, sgi.shading_normal, sgi.out_direction, sample.direction);
				vec3 sample_contribution = max(sample.intensity, vec3(0)) * bsdf * AbsDot(sgi.shading_normal, sample.direction);
				float bsdf_pdf = 0;
				const bool at_infinity = sample.flags & Light::ConnectionSample::Flags::AtInfinity;
				const bool sample_is_hitable = at_infinity;
				if(sample_is_hitable)
				{
					bsdf_pdf = material.pdf<BSDF_FORWARD_BIT>(sgi.geometry_normal, sgi.shading_normal, sgi.out_direction, sample.direction);
				}
				// weight / pdf -> rcp(sum(pdf))
				float mis_weight = rcp(bsdf_pdf + LIGHT_SAMPLES * sample.pdf);
				sample_contribution *= mis_weight;
				if(NonZero(sample_contribution))
				{
					Ray3f _ray = Ray3f(sgi.position, sample.direction);
					RayDesc ray = RayDesc(_ray, RayRangef(RayTMin(_ray, sgi.geometry_normal), sample.distance));
					float v = ray_tracer.testVisibility(ray);
					res += (sample_contribution * v);
				}
			}
		}
		return res * rcp(float(samples));
	}
}

vec3 TracePath<RNG : IRNG, RayTracer : IRayTracer>(const in BoundScene scene, const in RayTracer ray_tracer, TraversingRay tray, inout RNG rng)
{
	uint num_vertices = 1;
	const uint max_vertices = GetMaxDepth() + 1;
	vec3 res = vec3(0);
	// negative for delta sampling
	float ray_pdf = 1;

	LiSampler sampler_Li = LiSampler();
#ifdef USE_Li_RESAMPLING
	sampler_Li.resampling = rt_ubo.Li_resampling;
#else
	sampler_Li.resampling = 0;
#endif
	sampler_Li.scene = scene;

	DirectLightSurfaceShading<RayTracer> shader;
	shader.scene = scene;
	shader.ray_tracer = ray_tracer;
	shader.samples = LIGHT_SAMPLES;

	SurfaceGeometryInfo prev_sgi;
	PBMaterial prev_material;
	vec3 prev_beta;

	while(num_vertices <= max_vertices)
	{
		RayTraceInfo trace_info = {};
		trace_info.ray = tray.ray;
		trace_info.range = tray.range;
		trace_info.diffs = tray.ray_diffs;
		Hit hit = ray_tracer.traceRay<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES>(trace_info);
		if(!hit.hasValue())
		{
			vec3 Le = scene.getEnvLe(tray.ray.direction);
			vec3 contrib = tray.throughput * Le;
			if(NonZero(contrib))
			{
				float mis_weight = 1;
				if(num_vertices > 1 && ray_pdf > 0)
				{
					float pdfLi = sampler_Li.estimateEnvPdfLi(rng, prev_sgi, prev_material, tray.ray.direction, prev_beta, Le);
					mis_weight = ray_pdf * rcp(LIGHT_SAMPLES * pdfLi + ray_pdf);
				}
				res += contrib * mis_weight;
			}
			break;
		}
		++num_vertices;
		RayDiffTextureSampling sampling = RayDiffTextureSampling(hit.surface_diffs.uv_jacobians);
		PBMaterial material = scene.readMaterial(hit.material_id, hit.surface_shading_info.uv, true, sampling);
		ApplyTextureNormalIFN(hit.surface_shading_info, material);

		{
			let kappa = material.getKappa(hit.surface_shading_info.geometry_normal, -tray.ray.direction);
			if(kappa.hasValue)
			{
				tray.throughput *= exp2(-hit.t * kappa.value);
			}
		}

		if(num_vertices <= max_vertices)
		{
			res += FitWrongToZero(tray.throughput * shader.shade(sampler_Li, hit.surface_shading_info, material, tray.throughput, rng));
		}

		let bsdf_sample = material.sampleBSDF<BSDF_FORWARD_BIT>(
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

		ray_pdf = bsdf_sample.pdf;
		if(bsdf_sample.flags & BSDFSampleFlags::Delta)
		{
			ray_pdf = -ray_pdf;
		}
		tray.ray = Ray3f(hit.surface_shading_info.position, bsdf_sample.direction);
		tray.ray_diffs.origin_jacobian = hit.surface_diffs.position_jacobian;
		tray.ray_diffs.direction_jacobian = bsdf_sample.direction_jacobian;
		tray.range.resetRange();
		tray.range.min = RayTMin(tray.ray, hit.surface_shading_info.geometry_normal);
		const float cos_theta = Abs(Dot(hit.surface_shading_info.shading_normal, bsdf_sample.direction));
		prev_beta = tray.throughput;
		tray.throughput *= bsdf_sample.bsdf * cos_theta / bsdf_sample.pdf;
		tray.throughput = FitWrongToZero(tray.throughput);

		prev_sgi = hit.surface_shading_info;
		prev_material = material;
	}
	return res;
}

void CommonMain(uvec2 pixel, vec2 oo_dims, const in BoundScene scene, const in IRayTracer ray_tracer)
{
	uint seed = Hash(pixel);
	seed = seed ^ Hash(renderer_ubo.frame_idx);
	// Use the same seed for all pixel:
	//seed = Hash(renderer_ubo.frame_idx);
	RNG_t rng = RNG_t(seed);
	ICamera cam = MakeCamera(renderer_ubo.camera);
	vec3 res = vec3(0);

	_g_debug_signal = all(pixel == uvec2(512, 512));
	if(_g_debug_signal)
	{
		_g_debug_printer = DebugPrinter(vec3(pixel, 0), DEBUG_PIXEL_SPACE_BIT);
	}

	for(uint i = 0; i < PATH_PER_PIXEL; ++i)
	{
		// [0, 1]
		vec2 jitter = vec2(0);
		if(true)
		{
			jitter = rng.generate<float, 2>();
		}
		else
		{
			RNG_t _rng = RNG_t(Hash(renderer_ubo.frame_idx));
			jitter = _rng.generate<float, 2>();
		}
		const vec2 uv = (vec2(pixel) + jitter) * oo_dims;
		const vec2 cp = UVToClipSpace(uv);

		Matrix2f cp_jacobian = DiagonalMatrixV(oo_dims * 2);
		let cam_sample = cam.sampleWe(rng, cp, cp_jacobian);
		TraversingRay tray = TraversingRay(cam_sample);
		
		res += TracePath(scene, ray_tracer, tray, rng);
	}

	res *= rcp(float(PATH_PER_PIXEL));

	Target.Store(pixel, vec4(res, 1));
}


#if SHADER_SEMANTIC_COMPUTE

// 16x16 appears to be the optimum
#define LOCAL_SIZE_X 16
#define LOCAL_SIZE_Y 16
#define LOCAL_SIZE_Z 1

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
		RayQuerier ray_querier = RayQuerier(scene);

		CommonMain(pixel, oo_dims, scene, ray_querier);
	}
}

#endif

#if SHADER_SEMANTIC_RAYGEN

[shader("raygen")]
void main()
{
	const uvec2 pixel = DispatchRaysIndex().xy;
	const uvec2 dims = TextureSize(Target);
	const vec2 oo_dims = rcp(vec2(dims));
	if(all(pixel < dims))
	{
		BoundScene scene;
		RayTracer ray_tracer = RayTracer(scene);

		CommonMain(pixel, oo_dims, scene, ray_tracer);
	}
}

#endif