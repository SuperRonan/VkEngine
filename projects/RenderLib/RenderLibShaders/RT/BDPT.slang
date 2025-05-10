
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

#define FORCE_CAMERA_TYPE CAMERA_TYPE_PERSPECTIVE

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

// Smaller workgroups appear the be faster, this is because of the high memory bandwith
// 32 is fast
// 16 appears to be the fastest (although it is smaller than tha subgroup???)
#define LOCAL_SIZE_X 4
#define LOCAL_SIZE_Y 4
#define LOCAL_SIZE_Z 1

#define MAX_VERTICES (MAX_DEPTH + 2)

#define LOCAL_SIZE (LOCAL_SIZE_X * LOCAL_SIZE_Y * LOCAL_SIZE_Z)

// Some optimization ideas:

// Fast bool evaluation of a connection
// Shuffle connections in subgroup to sample only those with a contribution

// Compress Vertex memory footprint

// Store vertices pdf in shared memory for faster weight evaluation?

// Re-organize the connection loop for maximal reconvergeance
namespace bdpt
{
	struct Vertex
	{
		[Flags]
		enum Flags
		{
			None = 0x0,
			NonHitable = (0x1 << 0),
			AtInfinity = (0x1 << 1),
			DeltaScattering = (0x1 << 2),
			MaterialReflect = (0x1 << 3),
			MaterialTransmit = (0x1 << 4),
		};

		static Flags GetFlags(Camera::Sample::Flags sample_flags)
		{
			Flags res = Flags::None;
			if(sample_flags & (Camera::Sample::Flags::DeltaPosition | Camera::Sample::Flags::DeltaDirection))
			{
				res = res | Vertex::Flags::NonHitable;
			}
			// TODO Not connectable, but resamplable
			// if(sample_flags & Camera::Sample::Flags::DeltaDirection)
			// {
			// 	res = res | Vertex::Flags::NonConnectable;
			// }
			return res;
		}

		static Flags GetFlags(Light::Sample::Flags sample_flags)
		{
			Flags res = Flags::None;
			if(sample_flags & (Light::Sample::Flags::DeltaPosition | Light::Sample::Flags::DeltaDirection))
			{
				res = res | Vertex::Flags::NonHitable;
			}
			// TODO Not connectable, but resamplable
			// if(sample_flags & Light::Sample::Flags::DeltaDirection)
			// {
			// 	res = res | Vertex::Flags::NonConnectable;
			// }
			return res;
		}

		// world position
		// If the vertex is a infinity, holds the direction instead
		vec3 position;
		uint material_id;

		vec3 shading_normal;
		Flags flags;
		
		vec3 geometry_normal;
		float forward_pdf;

		vec3 beta;
		float reverse_pdf;

		vec2 uv;
		float connection_pdf;

		// Some optimization thoughts:
		// Maybe we could do some bit packing (store the index in 24 bits, and the flags on 8)
		// Maybe we could store only the ratio of pdfs, rather than the two separate ones
		// Maybe we could pack to the geometry normal and shading normal on a single vec4 size
		// We could lower the precision of the UVs
	};

uint RandomWalk<int Flags, VertexArray : IRWArray<Vertex>, RNG : IRNG>(
	in VertexArray storage, uint storage_offset, uint max_len,
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	TraversingRay tray,
	float pdf_solid_angle,
	inout RNG rng
) {
	uint res = 0;
	const bool light_tracer = (Flags & BSDF_ADJOINT_BIT) != 0;
	while(res < max_len)
	{
		RayTraceInfo trace_info = {};
		trace_info.ray = tray.ray;
		trace_info.range = tray.range;
		trace_info.diffs = tray.ray_diffs;
		Vertex v;
		Hit hit = ray_tracer.traceRay<RAY_FLAG_NONE>(trace_info);

		bool store_vertex = !light_tracer || hit.hasValue();
		
		if(store_vertex)
		{
			v.forward_pdf = pdf_solid_angle;
			v.reverse_pdf = 0;
			v.beta = tray.throughput;
			v.position = hit.surface_shading_info.position;
			v.material_id = hit.material_id;
			v.shading_normal = hit.surface_shading_info.shading_normal;
			v.geometry_normal = hit.surface_shading_info.geometry_normal;
			v.uv = hit.surface_shading_info.uv;
			v.flags = Vertex::Flags::None;
			// TODO check why the materials don't provide this info correctly
			v.flags = Vertex::Flags::MaterialReflect;
			
			if(hit.hasValue())
			{
				if(v.material_id < scene.getUBO().num_materials)
				{
					uint material_bsdf_flags = (ScenePBMaterialsProps[v.material_id][0].flags >> MATERIAL_FLAG_HEMISPHERE_BIT_OFFSET) & BitMask<uint>(2);
					v.flags = v.flags | reinterpret<Vertex::Flags>(material_bsdf_flags << 3);
				}
			}
			else
			{
				v.flags = v.flags | Vertex::Flags::AtInfinity;
			}
			storage[storage_offset + res] = v;
			++res;
		}
		else
		{
			break;
		}

		bool continue_path = hit.hasValue() && (res < max_len);

		if(continue_path)
		{
			float reverse_pdf = 0;
			
			PBMaterial material = scene.readMaterial(hit.material_id, hit.surface_shading_info.uv, true);
			const vec3 wo = -tray.ray.direction;
			const vec3 ns = hit.surface_shading_info.shading_normal;
			const vec3 ng = hit.surface_shading_info.geometry_normal;
			let bsdf_sample = material.sampleBSDF<Flags & BSDF_ADJOINT_BIT>(
				ng, ns, hit.surface_diffs.normal_jacobian,
				hit.surface_shading_info.out_direction,
				-tray.ray_diffs.direction_jacobian,
				rng
			);
			const vec3 wi = bsdf_sample.direction;
			const bool prev_non_hitable = (storage[storage_offset + res - 1].flags & Vertex::Flags::NonHitable) == 0;
			if(prev_non_hitable)
			{
				if(bsdf_sample.flags & BSDFSampleFlags::Delta)
				{
					// Assume delta reflections have symmetric pdf
					// Else, the samplerBSDF function should return the reverse pdf?
					reverse_pdf = bsdf_sample.pdf;
					storage[storage_offset + res - 1].flags = (storage[storage_offset + res - 1].flags | Vertex::Flags::DeltaScattering);
				}
				else
				{
					reverse_pdf = material.pdf<Flags & BSDF_ADJOINT_BIT>(ng, ns, wo, wi);	
				}
			}

			pdf_solid_angle = bsdf_sample.pdf;
			tray.ray = Ray3f(hit.surface_shading_info.position, wi);
			tray.ray_diffs.origin_jacobian = hit.surface_diffs.position_jacobian;
			tray.ray_diffs.direction_jacobian = bsdf_sample.direction_jacobian;
			tray.range.resetRange();
			tray.range.min = RayTMin(tray.ray, ng);
			const float cos_theta_i_s = Dot(ns, wi);
			const float cos_theta_i_g = Dot(ng, wi);
			const float cos_theta_o_g = Dot(ng, wo);
			const float cos_theta_o_s = Dot(ns, wo);
			const float correction = light_tracer ? (cos_theta_o_s * cos_theta_i_g) / (cos_theta_o_g * cos_theta_i_s) : 1.0f;
			tray.throughput *= (bsdf_sample.bsdf * Abs(cos_theta_i_s * correction) / bsdf_sample.pdf);
			
			storage[storage_offset + res - 1].reverse_pdf = reverse_pdf;
			
			if(!NonZero(tray.throughput) || pdf_solid_angle <= 0)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	return res;
}

uint TraceCameraSubPath<VertexArray : IRWArray<Vertex>, RNG : IRNG>(
	in VertexArray storage, uint storage_offset, uint max_len,
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	const in ICamera camera,
	const in vec2 cp,
	const in Matrix2f Jcp,
	uint layer,
	inout RNG rng
) {
	let sample = camera.sampleWe(rng, cp, Jcp, layer);
	TraversingRay tray = TraversingRay(sample);
	storage[storage_offset].position = sample.ray.ray.origin;
	storage[storage_offset].material_id = layer;
	storage[storage_offset].shading_normal = vec3(0);
	storage[storage_offset].flags = Vertex::GetFlags(sample.flags);
	storage[storage_offset].geometry_normal = vec3(0);
	storage[storage_offset].forward_pdf = sample.position_pdf;
	storage[storage_offset].beta = vec3(1) / sample.position_pdf;
	storage[storage_offset].reverse_pdf = 0;

	uint n = RandomWalk<BSDF_FORWARD_BIT>(storage, storage_offset + 1, max_len - 1, 
		scene, ray_tracer,
		tray,
		sample.pdf,
		rng
	);
	return n + 1;
}

uint TraceLightSubPath<VertexArray : IRWArray<Vertex>, RNG : IRNG>(
	in VertexArray storage, uint storage_offset, uint max_len,
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	inout RNG rng
) {
	let sample = scene.sampleLe(rng);
	Vertex::Flags flags = Vertex::GetFlags(sample.flags);

	TraversingRay tray = TraversingRay(sample);

	Vertex vertex;

	vertex.position = sample.ray.origin;
	vertex.material_id = sample.index;
	vertex.shading_normal = vec3(0);
	vertex.flags = flags;
	vertex.geometry_normal = vec3(0);
	vertex.forward_pdf = (sample.position_pdf);
	vertex.beta = vec3(1) / vertex.forward_pdf;
	vertex.reverse_pdf = 0;

	storage[storage_offset] = vertex;
	
	uint n = RandomWalk<BSDF_ADJOINT_BIT>(storage, storage_offset + 1, max_len - 1,
		scene, ray_tracer,
		tray,
		sample.pdf,
		rng
	);
	return n + 1;
}

layout(SHADER_DESCRIPTOR_BINDING + 3) restrict RWStructuredBuffer<Vector<LightBufferAccumFloat, 3>, Std430DataLayout> LightTracerAccumBuffer;

float VertexConnectionWeight<VertexArray : IArray<Vertex>>(
	const uint thread_index, const uint local_index,
	const in VertexArray vertices, 
	const uint camera_offset,
	const uint light_offset,
	const uint connection_t, const uint connection_s,
	const in Vertex connecting_camera, const in Vertex connecting_light,
	float pdfWi, float pdfLi,
	float xt_pdf_rev, float ys_pdf_rev,
	float xtm1_pdf_rev, float ysm1_pdf_rev,
	float num_lt_samples
) {
	float res = 1;

	float sum = 0;

	// Expand the camera subpath over the light subpath
	{
		float ri = 1;
		// st is the index of the expanding camera vertex (on the light subpath)
		uint min_s = 1;
		if(vertices[light_offset].flags & Vertex::Flags::NonHitable)
		{
			min_s = 2;
		}
		for(uint st = connection_s; st >= min_s; --st)
		{
			// s is the index of the connecting light vertex
			const uint s = st - 1;
			const bool full_cam_path = st == 1;
			const bool light_connection = st == 2;
			let camera_end = (connection_s == 1 && light_connection) ? connecting_light : vertices[light_offset + st - 1];
			float reverse_pdf;
			if(st == connection_s)	reverse_pdf = ys_pdf_rev;
			else if(st == connection_s - 1)	reverse_pdf = ysm1_pdf_rev;
			else reverse_pdf = camera_end.reverse_pdf;

			ri *= reverse_pdf / camera_end.forward_pdf;
			float actual_ri = ri;
			Vertex::Flags connection_flags = camera_end.flags;
			if(light_connection)
			{
				// Correct the pdf with pdfLi;
				float fwd_pdf = (connection_s == 1) ? connecting_light.forward_pdf : vertices[light_offset].forward_pdf;
				actual_ri *= pdfLi / fwd_pdf;
			}
			else if(s > 1)
			{
				connection_flags = connection_flags | vertices[light_offset + s - 1].flags;
			}
			// If connectible
			if((connection_flags & Vertex::Flags::DeltaScattering) == 0)
			{
				sum += actual_ri;
			}
		}
	}

	// Expand the light subpath over the camera subpath
	{
		float ri = 1;
		// Assume
		const bool cam_is_hitable = false;
		const uint min_t = cam_is_hitable ? 1 : 2;
		for(uint ts = connection_t; ts >= min_t; --ts)
		{
			const uint t = ts - 1;
			const bool light_connection = (connection_s == 0 && ts == connection_t);
			const bool light_tracer = (ts == 2);
			let light_end = (connection_t == 1 && light_tracer) ? connecting_camera : vertices[camera_offset + ts - 1];
			float reverse_pdf;
			if(ts == connection_t)	reverse_pdf = xt_pdf_rev;
			else if(ts == connection_t - 1)	reverse_pdf = xtm1_pdf_rev;
			else	reverse_pdf = light_end.reverse_pdf;
			ri *= reverse_pdf / light_end.forward_pdf;
			float actual_ri = ri;
			if(light_connection)
			{
				actual_ri *= pdfLi / reverse_pdf;
			}
			Vertex::Flags connection_flags = light_end.flags;
			if( t > 1)
			{
				connection_flags = connection_flags | vertices[camera_offset + t - 1].flags;
			}

			// If connectible
			bool can_be_sampled = ((connection_flags & Vertex::Flags::DeltaScattering) == 0);
			if(can_be_sampled)
			{
				const float ni = light_tracer ? num_lt_samples : 1;
				sum += actual_ri * ni;
			}
		}
	}

	float connection_pdf = 1.0f;

	if(connection_t == 1)
	{
		connection_pdf *= num_lt_samples;
	}
	res = connection_pdf * rcp(sum + connection_pdf);

	return res;
}

vec3 ConnectSubPathsAt<VertexArray : IArray<Vertex>>(
	const uint thread_index, const uint local_index,
	const in BoundScene scene,
	const in VertexArray vertices, 
	const uint camera_offset,
	const uint light_offset,
	const uint t, const uint s,
	const in Vertex camera_vertex, const in Vertex light_vertex,
	RayDesc visibility_ray,
	float pdfWi, float pdfLi,
	float xt_pdf_rev, float ys_pdf_rev,
	float xtm1_pdf_rev, float ysm1_pdf_rev,
	float num_lt_samples,
	float G,
	vec3 camera_connection, vec3 light_connection
) {
	if(t > 1 || s > 1)
	{
		if(t > 1 && s > 1)
		{
			vec3 tp = camera_vertex.position;
			vec3 sp = light_vertex.position;
			vec3 connection_vector = sp - tp;
			vec3 connection_dir = Normalize(connection_vector);
			float connection_d2 = Length2(connection_vector);
			visibility_ray.Origin = tp;
			visibility_ray.Direction = connection_dir;
			visibility_ray.TMin = RayTMin(Ray3f(tp, connection_dir), camera_vertex.geometry_normal);
			visibility_ray.TMax = Length(connection_vector);
			G *= rcp(connection_d2);
		}
		// Goes from camera to light
		vec3 connection_dir = (t == 1) ? -visibility_ray.Direction : visibility_ray.Direction;

		if(t > 1)
		{
			let material = scene.readMaterial(camera_vertex.material_id, camera_vertex.uv, true);
			vec3 prev_pos = vertices[camera_offset + t - 2].position;
			const vec3 wo = Normalize(prev_pos - camera_vertex.position);
			camera_connection = material.bsdf<BSDF_FORWARD_BIT>(camera_vertex.geometry_normal, camera_vertex.shading_normal, wo, connection_dir);
			ys_pdf_rev = material.pdf<BSDF_FORWARD_BIT>(camera_vertex.geometry_normal, camera_vertex.shading_normal, wo, connection_dir);
			xtm1_pdf_rev = material.pdf<BSDF_ADJOINT_BIT>(camera_vertex.geometry_normal, camera_vertex.shading_normal, connection_dir, wo);
			G *= AbsDot(camera_vertex.shading_normal, connection_dir);
		}
		if(s > 1)
		{
			let material = scene.readMaterial(light_vertex.material_id, light_vertex.uv, true);
			vec3 prev_pos = vertices[light_offset + s - 2].position;
			const vec3 wo = Normalize(prev_pos - light_vertex.position);
			const vec3 ns = light_vertex.shading_normal;
			const vec3 ng = light_vertex.geometry_normal;
			const vec3 wi = -connection_dir;
			const float cos_theta_i_s = Dot(ns, wi);
			const float cos_theta_i_g = Dot(ng, wi);
			const float cos_theta_o_g = Dot(ng, wo);
			const float cos_theta_o_s = Dot(ns, wo);
			// When connecting vertices on the same suface, cos_theta will be zero, but the correction term explodes to NaN
			const float correction = FitWrongToZero(Abs((cos_theta_o_s * cos_theta_i_g) / (cos_theta_o_g * cos_theta_i_s)));
			light_connection = material.bsdf<BSDF_ADJOINT_BIT>(light_vertex.geometry_normal, light_vertex.shading_normal, wo, -connection_dir);
			xt_pdf_rev = material.pdf<BSDF_ADJOINT_BIT>(light_vertex.geometry_normal, light_vertex.shading_normal, wo, -connection_dir);
			ysm1_pdf_rev = material.pdf<BSDF_FORWARD_BIT>(light_vertex.geometry_normal, light_vertex.shading_normal,-connection_dir, wo);
			G *= Abs(Dot(light_vertex.shading_normal, connection_dir) * correction);
		}
	}
	
	vec3 L = camera_vertex.beta * camera_connection * light_connection * light_vertex.beta * G;
	//L = FitWrongToZero(L);

	if(NonZero(L))
	{
		float v = scene.testVisibility(visibility_ray);
		L *= v;
		if(v > 0)
		{
			float w = VertexConnectionWeight(
				thread_index, local_index,
				vertices, 
				camera_offset,
				light_offset,
				t, s,
				camera_vertex, light_vertex,
				pdfWi, pdfLi,
				xt_pdf_rev, ys_pdf_rev,
				xtm1_pdf_rev, ysm1_pdf_rev,
				num_lt_samples
			);

			L *= w;
		}
	}
	return L;
}

vec3 ConnectSubPaths<RNG : IRNG, VertexArray : IArray<Vertex>>(
	const uint thread_index, const uint local_index,
	const in VertexArray vertices, 
	const uint camera_offset, const uint n_camera,
	const uint light_offset, const uint n_light,
	const in BoundScene scene,
	const in ICamera camera, vec2 fdims, uvec2 udims,
	inout RNG rng
) {
	vec3 res = vec3(0);
	const float num_lt_samples = 1;//Prod(fdims);
	// The camera subpath vertices are named x, and indexed by t
	// The light subpath vertices are named y, and indexed by s
	// t and s represent the number of vertices in each subpath (t for camera, s for light)
	// So this is a 1 based indexing (because each subpath might be empty)
	const bool cam_is_hitable = false;
	const uint t_min = cam_is_hitable ? 0 : 1;

	// TODO check the densitiy units
	// The resampled pdf of the first camera vertex when t == 1 (pdfWi)
	float t1_pdf_Wi = 0;
	// The resampled pdf of the first light vertex when s == 1 (pdfLi)
	float s1_pdf_Li = 0;
	
	for(uint t = t_min; t <= n_camera; ++t)
	{
		bool xt_at_infinity = (t == n_camera && (vertices[camera_offset + t - 1].flags & Vertex::Flags::AtInfinity) != 0);
		const uint max_s = xt_at_infinity ? 0 : min(n_light, MAX_VERTICES - t);
		const uint min_s = (t == 1 ? 1 : 0);
		for(uint s = min_s; s <= max_s; ++s)
		{
			vec3 L = 0;
			if(s == 0) // Full path tracer
			{
				// Don't handle this case now, lights are not hitable
				// TODO env map
			}
			else // Connection
			{
				// The pdf of xt being sampled by ys
				float xt_pdf_rev = 0;
				// The pdf of ys being sampled by xt
				float ys_pdf_rev = 0;
				// The pdf of xt-1 being sampled by xt (from ys)
				float xtm1_pdf_rev = 0;
				// The pdf of ys-1 being sampled by ys (fromt xt)
				float ysm1_pdf_rev = 0;

				const bool resample_light = s == 1;
				const bool resample_camera = t == 1;
				vec3 camera_connection = 0;
				vec3 light_connection = 0;
				float G = 1;
				bool connection_can_contribute = true;

				const Vertex storage_camera_vertex = vertices[camera_offset + t - 1];
				const Vertex storage_light_vertex = vertices[light_offset + s - 1];
				// xt
				Vertex camera_vertex = storage_camera_vertex;
				// ys
				Vertex light_vertex = storage_light_vertex;
				
				RayDesc visibility_ray;

				// When s == 1 && t == 1, it is a bit of a chicken and egg problem
				// Here it is solved implicitely from the fact that the camera is always sampled at the same position
				if(resample_light)
				{
					let light_sample = scene.sampleLi(rng, camera_vertex.position);
					Vertex::Flags flags = Vertex::GetFlags(light_sample.flags);
					light_vertex.flags = flags;
					// TODO Light Connection Sample could provide the exact position, rather than computing it (with a possible loss of precision)
					light_vertex.position = Ray3f(camera_vertex.position, light_sample.direction).at(light_sample.distance);
					light_vertex.material_id = light_sample.index;
					RayPdf pdfLe = scene.pdfLe(Ray3f(light_vertex.position, -light_sample.direction), light_sample.index);
					light_vertex.forward_pdf = pdfLe.position_pdf;
					light_vertex.reverse_pdf = light_sample.pdf;
					light_vertex.beta = vec3(1) * rcp(light_sample.pdf);
					s1_pdf_Li = light_sample.pdf * rcp(sqr(light_sample.distance));
					light_connection = light_sample.intensity;
					xt_pdf_rev = pdfLe.direction_pdf;
					
					visibility_ray.Origin = camera_vertex.position;
					visibility_ray.Direction = light_sample.direction;
					if(t == 1)
					{
						visibility_ray.TMin = 0;
					}
					else
					{
						visibility_ray.TMin = RayTMin(Ray3f(visibility_ray.Origin, visibility_ray.Direction), camera_vertex.geometry_normal);
					}
					visibility_ray.TMax = light_sample.distance;
				}
				else if(s == 2)
				{
					// This could be done when generating the sub paths?
					vec3 refp = light_vertex.position;
					vec3 dir = vertices[light_offset].position - refp;
					s1_pdf_Li = scene.pdfLi(refp, dir, vertices[light_offset].material_id) * rcp(Length2(dir));
				}

				if(resample_camera)
				{
					const vec3 light_pos = light_vertex.position;
					const vec3 ng = light_vertex.geometry_normal;
					let cam_sample = camera.sampleWi(rng, light_pos);
					camera_vertex.flags = Vertex::GetFlags(cam_sample.flags);
					camera_vertex.position = Ray3f(light_pos, cam_sample.direction).at(cam_sample.distance);
					RayPdf pdfWe = camera.pdfWe(Ray3f(camera_vertex.position, -cam_sample.direction));
					camera_vertex.forward_pdf = pdfWe.position_pdf;
					camera_vertex.reverse_pdf = cam_sample.pdf;
					camera_vertex.material_id = cam_sample.index;
					camera_vertex.beta = vec3(1) * rcp(cam_sample.pdf);
					camera_vertex.uv = cam_sample.uv;
					t1_pdf_Wi = cam_sample.pdf * rcp(sqr(cam_sample.distance));
					camera_connection = cam_sample.intensity;
					ys_pdf_rev = pdfWe.direction_pdf;

					visibility_ray.Origin = light_pos;
					visibility_ray.Direction = cam_sample.direction;
					visibility_ray.TMin = RayTMin(Ray3f(visibility_ray.Origin, visibility_ray.Direction), ng);
					visibility_ray.TMax = cam_sample.distance;
				}
				else if(t == 1)
				{
					// This could be done when generating the sub paths?
					vec3 refp = camera_vertex.position;
					vec3 dir = vertices[camera_offset].position - refp;
					t1_pdf_Wi = camera.pdfWi(refp, dir, vertices[camera_offset].material_id) * rcp(Length2(dir));
				}

				if(t > 1 || s > 1)
				{
					if(t > 1 && s > 1)
					{
						vec3 tp = camera_vertex.position;
						vec3 sp = light_vertex.position;
						vec3 connection_vector = sp - tp;
						vec3 connection_dir = Normalize(connection_vector);
						float connection_d2 = Length2(connection_vector);
						visibility_ray.Origin = tp;
						visibility_ray.Direction = connection_dir;
						visibility_ray.TMin = RayTMin(Ray3f(tp, connection_dir), camera_vertex.geometry_normal);
						visibility_ray.TMax = Length(connection_vector);
						G *= rcp(connection_d2);
					}
					// Goes from camera to light
					vec3 connection_dir = (t == 1) ? -visibility_ray.Direction : visibility_ray.Direction;
			
					if(t > 1)
					{
						let material = scene.readMaterial(camera_vertex.material_id, camera_vertex.uv, true);
						vec3 prev_pos = vertices[camera_offset + t - 2].position;
						const vec3 wo = Normalize(prev_pos - camera_vertex.position);
						camera_connection = material.bsdf<BSDF_FORWARD_BIT>(camera_vertex.geometry_normal, camera_vertex.shading_normal, wo, connection_dir);
						ys_pdf_rev = material.pdf<BSDF_FORWARD_BIT>(camera_vertex.geometry_normal, camera_vertex.shading_normal, wo, connection_dir);
						xtm1_pdf_rev = material.pdf<BSDF_ADJOINT_BIT>(camera_vertex.geometry_normal, camera_vertex.shading_normal, connection_dir, wo);
						G *= AbsDot(camera_vertex.shading_normal, connection_dir);
					}
					if(s > 1)
					{
						let material = scene.readMaterial(light_vertex.material_id, light_vertex.uv, true);
						vec3 prev_pos = vertices[light_offset + s - 2].position;
						const vec3 wo = Normalize(prev_pos - light_vertex.position);
						const vec3 ns = light_vertex.shading_normal;
						const vec3 ng = light_vertex.geometry_normal;
						const vec3 wi = -connection_dir;
						const float cos_theta_i_s = Dot(ns, wi);
						const float cos_theta_i_g = Dot(ng, wi);
						const float cos_theta_o_g = Dot(ng, wo);
						const float cos_theta_o_s = Dot(ns, wo);
						// When connecting vertices on the same suface, cos_theta will be zero, but the correction term explodes to NaN
						const float correction = FitWrongToZero(Abs((cos_theta_o_s * cos_theta_i_g) / (cos_theta_o_g * cos_theta_i_s)));
						light_connection = material.bsdf<BSDF_ADJOINT_BIT>(light_vertex.geometry_normal, light_vertex.shading_normal, wo, -connection_dir);
						xt_pdf_rev = material.pdf<BSDF_ADJOINT_BIT>(light_vertex.geometry_normal, light_vertex.shading_normal, wo, -connection_dir);
						ysm1_pdf_rev = material.pdf<BSDF_FORWARD_BIT>(light_vertex.geometry_normal, light_vertex.shading_normal,-connection_dir, wo);
						G *= Abs(Dot(light_vertex.shading_normal, connection_dir) * correction);
					}
				}
				
				vec3 L = camera_vertex.beta * camera_connection * light_connection * light_vertex.beta * G;
				//L = FitWrongToZero(L);
			
				if(NonZero(L))
				{
					float v = scene.testVisibility(visibility_ray);
					L *= v;
					if(v > 0)
					{
						float w = VertexConnectionWeight(
							thread_index, local_index,
							vertices, 
							camera_offset,
							light_offset,
							t, s,
							camera_vertex, light_vertex,
							t1_pdf_Wi, s1_pdf_Li,
							xt_pdf_rev, ys_pdf_rev,
							xtm1_pdf_rev, ysm1_pdf_rev,
							num_lt_samples
						);
			
						L *= w;
						
						if(t == 1)
						{
							AddLightTracerSample(LightTracerAccumBuffer, camera_vertex.uv, fdims, udims, L);
						}
						else
						{
							res += L;
						}
					}
				}
			} // if Connection
		} // for s
	} // for t

	return res;
}

layout(SHADER_DESCRIPTOR_BINDING + 4) restrict RWStructuredBuffer<Vertex, Std430DataLayout> SubPathsScratchBuffer;

vec3 PathTrace<RNG : IRNG>(
	const uint thread_index, const uint local_index,
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	const in ICamera camera, vec2 fdims, uvec2 udims,
	const in vec2 cp,
	const in Matrix2f Jcp,
	uint layer,
	inout RNG rng
) {
	uint storage_offset = thread_index * 2 * MAX_VERTICES;
	const uint cam_offset = storage_offset;
	const uint light_offset = storage_offset + MAX_VERTICES;

	// Assume these for now
	const bool lights_are_hitable = true;
	const bool camera_is_hitable = false;
	const uint max_camera = lights_are_hitable ? MAX_VERTICES : (MAX_VERTICES - 1);
	const uint max_light = camera_is_hitable ? MAX_VERTICES : (MAX_VERTICES - 1);

	uint n_camera = TraceCameraSubPath(
		SubPathsScratchBuffer, cam_offset, max_camera,
		scene, ray_tracer,
		camera, cp, Jcp, layer,
		rng
	);

	uint n_light = TraceLightSubPath(
		SubPathsScratchBuffer, light_offset, max_light,
		scene, ray_tracer,
		rng
	);

	vec3 res = ConnectSubPaths(
		thread_index, local_index,
		SubPathsScratchBuffer,
		cam_offset, n_camera,
		light_offset, n_light,
		scene,
		camera, fdims, udims,
		rng
	);

	return res;
}

} // namespace bdpt



[shader("compute")]
[numthreads(LOCAL_SIZE_X, LOCAL_SIZE_Y, LOCAL_SIZE_Z)]
void main(
	const uvec3 GlobalInvocationID : SV_DispatchThreadID,
	const uint3 LocalInvocationID : SV_GroupThreadID,
	const uint3 WorkGroupID : SV_GroupID
) {
	const uvec2 pixel = GlobalInvocationID.xy;
	const uvec2 dims = TextureSize(Target);
	const vec2 fdims = vec2(dims);
	const vec2 oo_dims = rcp(fdims);
	const uint local_thread_index = LocalInvocationID.x + LocalInvocationID.y * LOCAL_SIZE_X;
	const uint workgroup_index = WorkGroupID.x + WorkGroupID.y * DivUpSafe(dims.x, LOCAL_SIZE_X);
#if 0
	const uint global_thread_index = workgroup_index * LOCAL_SIZE + local_thread_index;
#else
	const uint global_thread_index = pixel.x + pixel.y * dims.x;
#endif
	if(all(pixel < dims))
	{
		BoundScene scene;

		RNG_t rng = RNG_t(Hash(pixel) ^ Hash(renderer_ubo.frame_idx));

		ICamera camera = MakeCamera(renderer_ubo.camera);
		RayQuerier ray_querier = RayQuerier(scene);
		
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
		Matrix2f Jcp = DiagonalMatrixV(oo_dims * 2);

		//_g_debug_signal = all(pixel == uvec2(1024, 34));
		if(_g_debug_signal)
		{
			_g_debug_printer = DebugPrinter(vec3(0), DEBUG_UV_SPACE_BIT);
			
		}

		vec3 res = bdpt::PathTrace(
			global_thread_index, local_thread_index, 
			scene, ray_querier, 
			camera, fdims, dims,
			cp, Jcp, 0, 
			rng
		);

		Target.Store(pixel, vec4(res, 1));
	}
}

