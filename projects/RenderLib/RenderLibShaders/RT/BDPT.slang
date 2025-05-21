
#define BIND_SCENE 1
#define BIND_SCENE_TLAS 1
#define BIND_ALL_SCENE_COMPONENTS 1
#define BIND_RENDERER_SET 1

#ifndef FORCE_COSINE_SAMPLING 
#define FORCE_COSINE_SAMPLING 0
#endif

#define I_WANT_TO_DEBUG 0
#define DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL I_WANT_TO_DEBUG
#define DEBUG_ENABLE_GLOBAL_PRINTER I_WANT_TO_DEBUG

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

#define LOCAL_SIZE (LOCAL_SIZE_X * LOCAL_SIZE_Y * LOCAL_SIZE_Z)

// 0 -> No compression
// 1 -> A little bit of compression: fastest
// 2 -> Aggressive compression: slower than 1
#define COMPRESS_VERTEX 1
#define COMPRESS_UV_WITH_HALF 1
#define UV_SCALE 8
#define USE_SEPARATE_PDF_STORAGE 1

// Some optimization ideas:

// Fast bool evaluation of a connection
// Shuffle connections in subgroup to sample only those with a contribution: Tried it, minimal improvement in the quality, but create noise correlation in the tiles

// Compress Vertex memory footprint : Done

// Store vertices pdf in shared memory for faster weight evaluation: Tried it: no perf improvment and forces to use compute shader

// Re-organize the connection loop for maximal reconvergeance

uint GetMaxVertices()
{
	return GetMaxDepth() + 2;
}

struct PDFPair
{
	float forward;
	float reverse;

	__init(float f, float r=0)
	{
		forward = f;
		reverse = r;
	}
};


struct DualNormalColorPack
{
	uvec4 data;

	property reference_dir : vec3
	{
		get {
			vec3 res;
			res.xy = unpackSnorm2x16ToFloat(data.x);
			res.z = unpackSnorm2x16ToFloat(data.y).x;
			return res;
		}
		set {
			data.x = packSnorm2x16(newValue.xy);
			data.y &= ~BitMask<u32>(16);
			data.y |= (packSnorm2x16(newValue.zz) & BitMask<u32>(16));
		}
	}

	property deviated_dir : vec3
	{
		get {
			vec3 n1 = this.reference_dir;
			mat3 B = BasisFromDir(n1, 1);
			vec2 diff = unpackSnorm2x16ToFloat(data.w);
			vec3 res = DiskToHemisphere(diff);
			return n1;
			return B * res;
		}
		set {
			vec3 n1 = this.reference_dir;
			mat3 B = BasisFromDir(n1, 1);
			vec3 in_basis = B.t * newValue;
			vec2 diff = in_basis.xy;
			data.w = packSnorm2x16(diff);
		}
	}

	property color : vec3
	{
		get {
			vec3 res;
			res.x = unpackSnorm2x16ToFloat(data.y).y;
			res.yz = unpackSnorm2x16ToFloat(data.z);
			return res;
		}
		set {
			data.y &= BitMask<u32>(16);
			data.y |= (packSnorm2x16(newValue.xx) & ~BitMask<u32>(16));
			data.z = packSnorm2x16(newValue.yz);
		}
	}
};

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

#if COMPRESS_VERTEX == 0
		vec3 position;
		PackedEnumIndex<Flags, 8, u32> packed_id_flags;

		vec3 beta;

		vec3 geometry_normal;
		float forward_pdf;

		vec3 shading_normal;
		float reverse_pdf;

		vec2 uv;
		
		property uint generic_index
		{
			get { return packed_id_flags.index; }
			set { packed_id_flags.index = newValue; }
		}

		property Flags flags
		{
			get { return packed_id_flags.enum; }
			set { packed_id_flags.enum = newValue; }
		}

		[mutating]
		void addFlags(Flags f)
		{
			packed_id_flags.payload |= (uint(f) << 24);
		}
#elif COMPRESS_VERTEX == 1
		vec3 position;
		PackedEnumIndex<Flags, 8, u32> packed_id_flags;

		vec3 geometry_normal;
#if COMPRESS_UV_WITH_HALF
		half2 uv;
#else
		u32 uv_pack;
		property uv : vec2
		{
			get{ return unpackSnorm2x16ToFloat(uv_pack) * UV_SCALE; }
			set{ uv_pack = packSnorm2x16(newValue * rcp(UV_SCALE)); }
		}
#endif
		
		half3 beta;
		half3 shading_normal;
		

		property uint generic_index
		{
			get { return packed_id_flags.index; }
			set { packed_id_flags.index = newValue; }
		}

		property Flags flags
		{
			get { return packed_id_flags.enum; }
			set { packed_id_flags.enum = newValue; }
		}

		[mutating]
		void addFlags(Flags f)
		{
			packed_id_flags.payload |= (uint(f) << 24);
		}

		
#else
		// world position
		// If the vertex is a infinity, holds the direction instead
		// Keep full f32 precision
		vec3 position;
		PackedEnumIndex<Flags, 8, u32> packed_id_flags;

		DualNormalColorPack normals_beta;
		
		// I prefer to keep full f32 precision for the pdfs
		float forward_pdf;
		float reverse_pdf;
		u32 uv_pack;

		property vec3 beta
		{
			//get { return normals_beta.color; }
			get { return normals_beta.color; }
			set { normals_beta.color = newValue; }
		}

		property vec3 geometry_normal
		{
			get { return normals_beta.reference_dir; }
			set { normals_beta.reference_dir = newValue; }
		}

		property vec3 shading_normal
		{
			get { return normals_beta.deviated_dir; }
			set { normals_beta.deviated_dir = newValue; }
		}

		property uint generic_index
		{
			get { return packed_id_flags.index; }
			set { packed_id_flags.index = newValue; }
		}

		property Flags flags
		{
			get { return packed_id_flags.enum; }
			set { packed_id_flags.enum = newValue; }
		}

		[mutating]
		void addFlags(Flags f)
		{
			packed_id_flags.payload |= (uint(f) << 24);
		}

		property uv : vec2
		{
			get{ return unpackSnorm2x16ToFloat(uv_pack) * UV_SCALE; }
			set{ uv_pack = packSnorm2x16(newValue * rcp(UV_SCALE)); }
		}
#endif
	};

uint RandomWalk<int Flags, VertexArray : IRWArray<Vertex>, RNG : IRNG, PDFStorage : IRWArray<PDFPair>>(
	in VertexArray storage, uint storage_offset, uint max_len,
	in PDFStorage storage_pdf, uint storage_pdf_offset,
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	TraversingRay tray, float prev_cos_theta,
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

		float reverse_geometric_conversion;
		
		if(store_vertex)
		{
			const uint material_id = hit.material_id & BitMask<uint>(24);
			const float rcp_d2 = rcp(sqr(hit.t));
			const float forward_geometric_conversion = AbsDot(hit.surface_shading_info.geometry_normal, tray.ray.direction) * rcp_d2;
			reverse_geometric_conversion = prev_cos_theta * rcp_d2;
			storage_pdf[storage_pdf_offset + res] = PDFPair(pdf_solid_angle * forward_geometric_conversion);
			v.beta = tray.throughput;
			v.position = hit.surface_shading_info.position;
			v.generic_index = material_id;
			v.geometry_normal = hit.surface_shading_info.geometry_normal;
			v.shading_normal = hit.surface_shading_info.shading_normal;
			v.uv = hit.surface_shading_info.uv;
			v.flags = Vertex::Flags::None;
			// TODO check why the materials don't provide this info correctly
			v.flags = Vertex::Flags::MaterialReflect;
			
			if(hit.hasValue())
			{
				{
					let material = scene.readMaterial(hit.material_id, hit.surface_shading_info.uv, true);
					if(ApplyTextureNormalIFN(hit.surface_shading_info, material))
					{
						v.shading_normal = hit.surface_shading_info.shading_normal;
					}
					let kappa = material.getKappa(hit.surface_shading_info.geometry_normal, -tray.ray.direction);
					if(kappa.hasValue)
					{
						tray.throughput *= exp2(-hit.t * kappa.value);
						v.beta = tray.throughput;
					}
				}
				if(material_id < scene.getUBO().num_materials)
				{
					uint material_bsdf_flags = (ScenePBMaterialsProps[material_id][0].flags >> MATERIAL_FLAG_HEMISPHERE_BIT_OFFSET) & BitMask<uint>(2);
					v.addFlags(reinterpret<Vertex::Flags>(material_bsdf_flags << 3));
				}
			}
			else
			{
				v.addFlags(Vertex::Flags::AtInfinity);
				v.position = tray.ray.direction;
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
			const bool prev_non_hitable = (storage[storage_offset + res - 2].flags & Vertex::Flags::NonHitable) == 0;
			if(prev_non_hitable)
			{
				if(bsdf_sample.flags & BSDFSampleFlags::Delta)
				{
					// Assume delta reflections have symmetric pdf
					// Else, the samplerBSDF function should return the reverse pdf?
					reverse_pdf = bsdf_sample.pdf;
					// Mark delta pdf with a negative sign
					storage_pdf[storage_pdf_offset + res - 1].forward = -storage_pdf[storage_pdf_offset + res - 1].forward;
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
			prev_cos_theta = Abs(cos_theta_i_g);
			tray.throughput *= (bsdf_sample.bsdf * Abs(cos_theta_i_s * correction) / bsdf_sample.pdf);
			tray.throughput = FitWrongToZero(tray.throughput);
			
			storage_pdf[storage_pdf_offset + res - 2].reverse = reverse_pdf * reverse_geometric_conversion;
			
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

uint TraceCameraSubPath<VertexArray : IRWArray<Vertex>, RNG : IRNG, PDFStorage : IRWArray<PDFPair>>(
	in VertexArray storage, uint storage_offset, uint max_len,
	in PDFStorage storage_pdf, uint storage_pdf_offset,
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	const in vec2 cp,
	const in Matrix2f Jcp,
	uint layer,
	inout RNG rng
) {
	let camera = MakeCamera(rt_ubo.camera);
	let sample = camera.sampleWe(rng, cp, Jcp, layer);
	TraversingRay tray = TraversingRay(sample);
	storage[storage_offset].position = sample.ray.ray.origin;
	storage[storage_offset].generic_index = layer;
	storage[storage_offset].flags = Vertex::GetFlags(sample.flags);
	storage_pdf[storage_pdf_offset] = PDFPair(sample.position_pdf);
	storage[storage_offset].beta = vec3(1) / sample.position_pdf;

	uint n = RandomWalk<BSDF_FORWARD_BIT>(
		storage, storage_offset + 1, max_len - 1,
		storage_pdf, storage_pdf_offset + 1,
		scene, ray_tracer,
		tray, 1,
		sample.pdf,
		rng
	);
	return n + 1;
}

uint TraceLightSubPath<VertexArray : IRWArray<Vertex>, RNG : IRNG, PDFStorage : IRWArray<PDFPair>>(
	in VertexArray storage, uint storage_offset, uint max_len,
	in PDFStorage storage_pdf, uint storage_pdf_offset,
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	inout RNG rng
) {
	let sample = scene.sampleLe(rng);
	Vertex::Flags flags = Vertex::GetFlags(sample.flags);

	TraversingRay tray = TraversingRay(sample);

	Vertex vertex;

	vertex.position = sample.ray.origin;
	vertex.generic_index = sample.index;
	vertex.flags = flags;
	storage_pdf[storage_pdf_offset] = PDFPair(sample.position_pdf);
	vertex.beta = vec3(1) / sample.position_pdf;

	storage[storage_offset] = vertex;
	
	uint n = RandomWalk<BSDF_ADJOINT_BIT>(
		storage, storage_offset + 1, max_len - 1,
		storage_pdf, storage_pdf_offset + 1,
		scene, ray_tracer,
		tray, 1,
		sample.pdf,
		rng
	);
	return n + 1;
}

layout(SHADER_DESCRIPTOR_BINDING + 3) restrict RWStructuredBuffer<Vector<LightBufferAccumFloat, 3>, Std430DataLayout> LightTracerAccumBuffer;

float VertexConnectionWeight<VertexArray : IArray<Vertex>, PDFStorage : IRWArray<PDFPair>>(
	const uint thread_index,
	const in VertexArray vertices,
	const uint camera_offset,
	const uint light_offset,
	const in PDFStorage storage_pdf, uint camera_pdf_offset, uint light_pdf_offset,
	const uint connection_t, const uint connection_s,
	const in PDFPair resampled_camera, const in PDFPair resampled_light,
	float pdfWi, float pdfLi,
	float xt_pdf_rev, float ys_pdf_rev,
	float xtm1_pdf_rev, float ysm1_pdf_rev,
	float num_lt_samples
) {
	float res = 1;
	//return res;
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
			const bool x_is_ys = st == connection_s;
			const bool x_is_ysm1 = st == connection_s - 1;
			
			PDFPair pdfs;
			
			PDFPair camera_end_pdfs = (connection_s == 1 && light_connection) ? resampled_light : storage_pdf[light_pdf_offset + st - 1];
			if(st == connection_s)	camera_end_pdfs.reverse = ys_pdf_rev;
			else if(st == connection_s - 1)	camera_end_pdfs.reverse = ysm1_pdf_rev;

			//camera_end_pdfs.forward = 1;
			//camera_end_pdfs.reverse = 1;
			
			bool can_connect = ExtractSignBit(camera_end_pdfs.forward) == false;

			ri *= camera_end_pdfs.reverse / abs(camera_end_pdfs.forward);
			float actual_ri = ri;
			if(light_connection)
			{
				// Correct the pdf with pdfLi;
				float forward_pdf = (connection_s == 1) ? resampled_light.forward : storage_pdf[light_pdf_offset].forward;
				actual_ri *= pdfLi / abs(forward_pdf);
			}
			else if(s > 1 && can_connect)
			{
				can_connect = ExtractSignBit(storage_pdf[light_pdf_offset + s - 1].forward) == false;
			}
			
			if(can_connect)
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
			const bool y_is_xt = (ts == connection_t);
			const bool y_is_xtm1 = (ts == connection_t - 1);
			PDFPair light_end_pdfs = (connection_t == 1 && light_tracer) ? resampled_camera : storage_pdf[camera_pdf_offset + ts - 1];
			if(ts == connection_t)	light_end_pdfs.reverse = xt_pdf_rev;
			else if(ts == connection_t - 1)	light_end_pdfs.reverse = xtm1_pdf_rev;
			bool can_connect = ExtractSignBit(light_end_pdfs.forward) == false;

			//light_end_pdfs.forward = 1;
			//light_end_pdfs.reverse = 1;

			ri *= light_end_pdfs.reverse / abs(light_end_pdfs.forward);
			float actual_ri = ri;
			
			if(light_connection)
			{
				float forward_pdf = (connection_t == 1) ? resampled_camera.forward : storage_pdf[camera_pdf_offset].forward;
				actual_ri *= pdfWi / abs(forward_pdf);
			}
			if( t > 1 && can_connect)
			{
				can_connect = ExtractSignBit(storage_pdf[camera_pdf_offset + t - 1].forward) == false;
			}

			if(can_connect)
			{
				const float ni = light_tracer ? num_lt_samples : 1;
				sum += actual_ri * ni;
			}
		}
	}

	float connection_pdf = 1.0f;
	if(connection_t == 1)
	{
		connection_pdf *= pdfWi / abs(resampled_camera.forward);
	}
	if(connection_s == 1)
	{
		connection_pdf *= pdfLi / abs(resampled_light.forward);
	}
	res = connection_pdf * rcp(sum + connection_pdf);

	return res;
}

vec3 ConnectSubPaths<RNG : IRNG, VertexArray : IArray<Vertex>, PDFStorage : IRWArray<PDFPair>>(
	const uint thread_index,
	const in VertexArray vertices, 
	const uint camera_offset, const uint n_camera,
	const uint light_offset, const uint n_light,
	const in PDFStorage pdf_storage, uint camera_pdf_offset, uint light_pdf_offset,
	const in BoundScene scene, const in IRayTracer ray_tracer,
	const vec2 fdims, const uvec2 udims,
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
		const uint max_s = xt_at_infinity ? 0 : min(n_light, GetMaxVertices() - t);
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
				PDFPair camera_vertex_pdfs = pdf_storage[camera_pdf_offset + t - 1];
				// ys
				Vertex light_vertex = storage_light_vertex;
				PDFPair light_vertex_pdfs = pdf_storage[light_pdf_offset + s - 1];
				
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
					light_vertex.generic_index = light_sample.index;
					RayPdf pdfLe = scene.pdfLe(Ray3f(light_vertex.position, -light_sample.direction), light_sample.index);
					light_vertex_pdfs = PDFPair(pdfLe.position_pdf);
					light_vertex.beta = vec3(1) * rcp(light_sample.pdf);

					const float rcp_d2 = rcp(sqr(light_sample.distance));

					s1_pdf_Li = light_sample.pdf * rcp_d2;
					light_connection = light_sample.intensity;
					xt_pdf_rev = pdfLe.direction_pdf * rcp_d2 * AbsDot(light_sample.direction, camera_vertex.geometry_normal);
					
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
					s1_pdf_Li = scene.pdfLi(refp, dir, vertices[light_offset].generic_index) * rcp(Length2(dir));
				}

				vec2 light_tracer_uv;

				if(resample_camera)
				{
					let camera = MakeCamera(rt_ubo.camera);
					const vec3 light_pos = light_vertex.position;
					const vec3 ng = light_vertex.geometry_normal;
					let cam_sample = camera.sampleWi(rng, light_pos);
					camera_vertex.flags = Vertex::GetFlags(cam_sample.flags);
					camera_vertex.position = Ray3f(light_pos, cam_sample.direction).at(cam_sample.distance);
					RayPdf pdfWe = camera.pdfWe(Ray3f(camera_vertex.position, -cam_sample.direction));
					camera_vertex_pdfs = PDFPair(pdfWe.position_pdf);
					camera_vertex.generic_index = cam_sample.index;
					camera_vertex.beta = vec3(1) * rcp(cam_sample.pdf);
					light_tracer_uv = cam_sample.uv;

					const float rcp_d2 = rcp(sqr(cam_sample.distance));

					t1_pdf_Wi = cam_sample.pdf * rcp_d2;
					camera_connection = cam_sample.intensity;
					ys_pdf_rev = pdfWe.direction_pdf * rcp_d2 * AbsDot(cam_sample.direction, light_vertex.geometry_normal);

					visibility_ray.Origin = light_pos;
					visibility_ray.Direction = cam_sample.direction;
					visibility_ray.TMin = RayTMin(Ray3f(visibility_ray.Origin, visibility_ray.Direction), ng);
					visibility_ray.TMax = cam_sample.distance;
				}
				else if(t == 2)
				{
					let camera = MakeCamera(rt_ubo.camera);
					// This could be done when generating the sub paths?
					vec3 refp = camera_vertex.position;
					vec3 dir = vertices[camera_offset].position - refp;
					t1_pdf_Wi = camera.pdfWi(refp, dir, vertices[camera_offset].generic_index) * rcp(Length2(dir));
				}

				if(t > 1 || s > 1)
				{
					float connection_d2;
					if(t > 1 && s > 1)
					{
						vec3 tp = camera_vertex.position;
						vec3 sp = light_vertex.position;
						vec3 connection_vector = sp - tp;
						vec3 connection_dir = Normalize(connection_vector);
						connection_d2 = Length2(connection_vector);
						visibility_ray.Origin = tp;
						visibility_ray.Direction = connection_dir;
						visibility_ray.TMin = RayTMin(Ray3f(tp, connection_dir), camera_vertex.geometry_normal);
						visibility_ray.TMax = Length(connection_vector) - RayTMin(Ray3f(sp, -connection_dir), light_vertex.geometry_normal);
						G *= rcp(connection_d2);
					}
					// Goes from camera to light
					vec3 connection_dir = (t == 1) ? -visibility_ray.Direction : visibility_ray.Direction;
			
					if(t > 1)
					{
						let material = scene.readMaterial(camera_vertex.generic_index, camera_vertex.uv, true);
						let prev_vertex = vertices[camera_offset + t - 2];
						vec3 prev_pos = prev_vertex.position;
						const float prev_d2 = Length2(prev_pos - camera_vertex.position);
						const vec3 wo = Normalize(prev_pos - camera_vertex.position);
						camera_connection = material.bsdf<BSDF_FORWARD_BIT>(camera_vertex.geometry_normal, camera_vertex.shading_normal, wo, connection_dir);
						ys_pdf_rev = material.pdf<BSDF_FORWARD_BIT>(camera_vertex.geometry_normal, camera_vertex.shading_normal, wo, connection_dir) * AbsDot(connection_dir, light_vertex.geometry_normal) / connection_d2;
						xtm1_pdf_rev = material.pdf<BSDF_ADJOINT_BIT>(camera_vertex.geometry_normal, camera_vertex.shading_normal, connection_dir, wo) * AbsDot(wo, prev_vertex.geometry_normal) / prev_d2;
						G *= AbsDot(camera_vertex.shading_normal, connection_dir);
					}
					if(s > 1)
					{
						let material = scene.readMaterial(light_vertex.generic_index, light_vertex.uv, true);
						let prev_vertex = vertices[light_offset + s - 2];
						vec3 prev_pos = prev_vertex.position;
						const float prev_d2 = Length2(prev_pos - light_vertex.position);
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
						xt_pdf_rev = material.pdf<BSDF_ADJOINT_BIT>(light_vertex.geometry_normal, light_vertex.shading_normal, wo, -connection_dir) * AbsDot(connection_dir, camera_vertex.geometry_normal) / connection_d2;
						ysm1_pdf_rev = material.pdf<BSDF_FORWARD_BIT>(light_vertex.geometry_normal, light_vertex.shading_normal,-connection_dir, wo) * AbsDot(wo, prev_vertex.geometry_normal) / prev_d2;
						G *= Abs(Dot(light_vertex.shading_normal, connection_dir) * correction);
					}
				}
				
				vec3 L = camera_vertex.beta * camera_connection * light_connection * light_vertex.beta * G;
				L = FitWrongToZero(L);
			
				if(NonZero(L))
				{
#if SHADER_SEMANTIC_COMPUTE
					float v = ray_tracer.testVisibility(visibility_ray);
#else
					// The visibility ray is broken with a RT Pipeline
					float v = 1.0f;
#endif
					L *= v;
					if(v > 0)
					{
						float w = VertexConnectionWeight(
							thread_index, 
							vertices, 
							camera_offset,
							light_offset,
							pdf_storage, camera_pdf_offset, light_pdf_offset,
							t, s,
							camera_vertex_pdfs, light_vertex_pdfs,
							t1_pdf_Wi, s1_pdf_Li,
							xt_pdf_rev, ys_pdf_rev,
							xtm1_pdf_rev, ysm1_pdf_rev,
							num_lt_samples
						);
			
						L *= w;
						
						if(t == 1)
						{
							AddLightTracerSample(LightTracerAccumBuffer, light_tracer_uv, fdims, udims, L);
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

layout(SHADER_DESCRIPTOR_BINDING + 5) restrict RWStructuredBuffer<PDFPair, Std430DataLayout> SubPathsPDFScratchBuffer;

vec3 PathTrace<RNG : IRNG>(
	const uint thread_index,
	const in BoundScene scene,
	const in IRayTracer ray_tracer,
	const vec2 fdims, const uvec2 udims,
	const in vec2 cp,
	const in Matrix2f Jcp,
	uint layer,
	inout RNG rng
) {
	const uint max_vertices = GetMaxVertices();
	// Assume these for now
	const bool lights_are_hitable = true;
	const bool camera_is_hitable = false;
	const uint max_camera = lights_are_hitable ? max_vertices : (max_vertices - 1);
	const uint max_light = camera_is_hitable ? max_vertices : (max_vertices - 1);

	const uint needed_vertices = max_camera + max_light;

	uint storage_offset = thread_index * needed_vertices;
	const uint cam_offset = storage_offset;
	const uint light_offset = storage_offset + max_camera;

	const uint camera_pdf_offset = cam_offset;
	const uint light_pdf_offset = light_offset;


	uint n_camera = TraceCameraSubPath(
		SubPathsScratchBuffer, cam_offset, max_camera,
		SubPathsPDFScratchBuffer, camera_pdf_offset,
		scene, ray_tracer,
		cp, Jcp, layer,
		rng
	);

	uint n_light = TraceLightSubPath(
		SubPathsScratchBuffer, light_offset, max_light,
		SubPathsPDFScratchBuffer, light_pdf_offset,
		scene, ray_tracer,
		rng
	);

	vec3 res = ConnectSubPaths(
		thread_index,
		SubPathsScratchBuffer,
		cam_offset, n_camera,
		light_offset, n_light,
		SubPathsPDFScratchBuffer, camera_pdf_offset, light_pdf_offset,
		scene, ray_tracer,
		fdims, udims,
		rng
	);

	return res;
}

} // namespace bdpt


void CommonMain(
	uvec2 pixel,
	const in IRayTracer ray_tracer,
	const in BoundScene scene,

) {
	const uvec2 dims = TextureSize(Target);
	const vec2 fdims = vec2(dims);
	const vec2 oo_dims = rcp(fdims);

// row major thread index appears to be the fastest.
#if 0
	const uint global_thread_index = workgroup_index * LOCAL_SIZE + local_thread_index;
#else
	const uint global_thread_index = pixel.x + pixel.y * dims.x;
#endif

	uint seed = Hash(pixel);
	seed = seed ^ Hash(renderer_ubo.frame_idx);
	RNG_t rng = RNG_t(seed);

	ICamera camera = MakeCamera(renderer_ubo.camera);
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

	vec3 res = bdpt::PathTrace(
		global_thread_index,
		scene, ray_tracer, 
		fdims, dims,
		cp, Jcp, 0, 
		rng
	);
	
	Target.Store(pixel, vec4(res, 1));
}

#if SHADER_SEMANTIC_COMPUTE
// Smaller workgroups appear the be faster, this is because of the high memory bandwith
// 32 is fast
// 16 appears to be the fastest (although it is smaller than a subgroup???)
#define LOCAL_SIZE_X 4
#define LOCAL_SIZE_Y 4
#define LOCAL_SIZE_Z 1
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

	if(all(pixel < dims))
	{
		BoundScene scene;
		RayQuerier ray_querier = RayQuerier(scene);
		
		CommonMain(pixel, ray_querier, scene);
	}
}
#endif


#if SHADER_SEMANTIC_RAYGEN
[shader("raygen")]
void main() {
	const uvec2 pixel = DispatchRaysIndex().xy;
	const uvec2 dims = TextureSize(Target);
	const vec2 fdims = vec2(dims);
	const vec2 oo_dims = rcp(fdims);

	if(all(pixel < dims))
	{
		BoundScene scene;
		RayTracer ray_tracer = RayTracer(scene);
		
		CommonMain(pixel, ray_tracer, scene);
	}
}
#endif