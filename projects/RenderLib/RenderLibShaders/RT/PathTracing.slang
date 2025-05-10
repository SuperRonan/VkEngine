
#define BIND_SCENE 1
#define BIND_SCENE_TLAS 1
#define BIND_ALL_SCENE_COMPONENTS 1
#define BIND_RENDERER_SET 1

#ifndef FORCE_COSINE_SAMPLING 
#define FORCE_COSINE_SAMPLING 0
#endif

#define DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL 1
#define DEBUG_ENABLE_GLOBAL_PRINTER 1
#define I_WANT_TO_DEBUG 0

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

// 16x16 appears to be the optimum
#define LOCAL_SIZE_X 16
#define LOCAL_SIZE_Y 16
#define LOCAL_SIZE_Z 1

vec3 TracePath<RNG : IRNG>(const in BoundScene scene, const in IRayTracer ray_tracer, TraversingRay tray, inout RNG rng)
{
	uint num_vertices = 1;
	const uint max_vertices = MAX_DEPTH + 1;
	vec3 res = vec3(0);
	while(num_vertices < max_vertices)
	{
		RayTraceInfo trace_info = {};
		trace_info.ray = tray.ray;
		trace_info.range = tray.range;
		trace_info.diffs = tray.ray_diffs;
		Hit hit = ray_tracer.traceRay<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES>(trace_info);
		if(!hit.hasValue())
		{
			break;
		}
		++num_vertices;
		RayDiffTextureSampling sampling = RayDiffTextureSampling(hit.surface_diffs.uv_jacobians);
		PBMaterial material = scene.readMaterial(hit.material_id, hit.surface_shading_info.uv, true, sampling);

		DirectLightSurfaceShading shader;
		shader.scene = scene;
		shader.samples = LIGHT_SAMPLES;
		res += tray.throughput * shader.shade(hit.surface_shading_info, material, rng);

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

		tray.ray = Ray3f(hit.surface_shading_info.position, bsdf_sample.direction);
		tray.ray_diffs.origin_jacobian = hit.surface_diffs.position_jacobian;
		tray.ray_diffs.direction_jacobian = bsdf_sample.direction_jacobian;
		tray.range.resetRange();
		tray.range.min = RayTMin(tray.ray, hit.surface_shading_info.geometry_normal);
		const float cos_theta = Abs(Dot(hit.surface_shading_info.shading_normal, bsdf_sample.direction));
		tray.throughput *= bsdf_sample.bsdf * cos_theta / bsdf_sample.pdf;
	}
	return res;
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

		ICamera cam = MakeCamera(renderer_ubo.camera);
		RayQuerier ray_querier = RayQuerier(scene);
		
		vec3 res = vec3(0);

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
			
			res += TracePath(scene, ray_querier, tray, rng);
		}

		res *= rcp(float(PATH_PER_PIXEL));

		Target.Store(pixel, vec4(res, 1));
	}
}

