#pragma once

#include <ShaderLib/Texturing.slang>

#include <ShaderLib/Rendering/Lights/Lights.slang>
#include <ShaderLib/Rendering/Materials/PBMaterial.slang>
#include <ShaderLib/Rendering/Geometry/Mesh.slang>

#include <ShaderLib/RayTracingCommon.slang>

#include <ShaderLib/Rendering/Lights/SkyLight.slang>

#ifndef BIND_ALL_SCENE_COMPONENTS
#define BIND_ALL_SCENE_COMPONENTS 1
#endif

#include "SceneDefinitions.h"

interface IScene
{



};

interface IRayTracingScene : IScene
{

};

struct SurfaceGeometryInfo
{
	vec3 position = {};
	vec3 geometry_normal = {};
	vec3 vertex_shading_normal = {};
	vec3 shading_normal = {};
	vec3 vertex_shading_tangent = {};
	vec3 out_direction = {};

	// Not re-normalized
	vec3 getShadingBiTangent()
	{
		vec3 res = (Cross(vertex_shading_tangent, vertex_shading_normal));
		return res;
	}

	Matrix3f getShadingTBN()
	{
		return MakeFromCols(vertex_shading_tangent, getShadingBiTangent(), shading_normal);
	}

	__init() {};
};

struct SurfaceShadingInfo : SurfaceGeometryInfo
{
	// In world space, not normalized
	vec3 interpolated_normal = {};
	vec2 uv = {};

	__init() {}
};

struct Scene
{
	struct UBO
	{
		uint num_lights;
		uint num_objects;
		uint num_mesh;
		uint num_materials;

		uint num_textures;

		vec3 ambient;

		vec3 sky;

		vec3 solar_direction;
		float solar_disk_cosine;

		vec3 solar_disk_intensity;
		float solar_disk_radius;

		vec3 center;
		float radius;
	};

	struct ObjectReference
	{
		uint mesh_id;
		uint material_id;
		uint xform_id;
		uint flags;
	};

	float _alpha_threshold = 0.1;

	float getOpaqueAlphaThreshold()
	{
		return _alpha_threshold;
	}

	bool testOpacity(float alpha)
	{
		return alpha < getOpaqueAlphaThreshold();
	}
};

#if BIND_SCENE

layout(SCENE_BINDING + 0) ConstantBuffer<Scene::UBO> SceneUBO;

#ifdef SCENE_LIGHTS_ACCESS
typealias SceneLightsBufferType = BINDING_HANDLE(StructuredBuffer, SCENE_LIGHTS_ACCESS)<StorageLight, Std430DataLayout>;
layout(SCENE_LIGHTS_BINDING + 0) restrict SceneLightsBufferType SceneLights;

layout(SCENE_LIGHTS_BINDING + 1) uniform Texture2D<float> SceneLightsDepth2D[];
layout(SCENE_LIGHTS_BINDING + 2) uniform TextureCube<float> SceneLightsDepthCube[];

#endif

#ifdef SCENE_OBJECTS_ACCESS
typealias SceneObjectsTableType = BINDING_HANDLE(StructuredBuffer, SCENE_OBJECTS_ACCESS)<Scene::ObjectReference>;
layout(SCENE_OBJECTS_BINDING + 0) restrict SceneObjectsTableType SceneObjectsTable;
#endif

#ifdef SCENE_MESH_ACCESS
typealias SceneMeshsHeadersType = BINDING_HANDLE(StructuredBuffer, SCENE_MESH_ACCESS)<Mesh::Header, Std430DataLayout>;
typealias SceneMeshsVerticesType = BINDING_HANDLE(StructuredBuffer, SCENE_MESH_ACCESS)<Mesh::Vertex, Std430DataLayout>;
typealias SceneMeshsIndicesType = BINDING_HANDLE(ByteAddressBuffer, SCENE_MESH_ACCESS);
layout(SCENE_MESHS_BINDING + 0) restrict SceneMeshsHeadersType SceneMeshHeaders[];
layout(SCENE_MESHS_BINDING + 1) restrict SceneMeshsVerticesType SceneMeshVertices[];
layout(SCENE_MESHS_BINDING + 2) restrict SceneMeshsIndicesType SceneMeshIndices[];
#endif

#ifdef SCENE_MATERIAL_ACCESS
typealias ScenePBMaterialsPropertiesType = BINDING_HANDLE(StructuredBuffer, SCENE_MATERIAL_ACCESS)<PBMaterialProperties>;
typealias SceneMaterialsTexturesType = BINDING_HANDLE(ByteAddressBuffer, SCENE_MATERIAL_ACCESS);
layout(SCENE_MATERIAL_BINDING + 0) restrict ScenePBMaterialsPropertiesType ScenePBMaterialsProps[];
layout(SCENE_MATERIAL_BINDING + 1) restrict SceneMaterialsTexturesType SceneMaterialsTexturesIds;
#endif

#if BIND_SCENE_TEXTURES
layout(SCENE_TEXTURES_BINDING + 0) uniform Sampler2D<float4> SceneTextures2D[];
#endif

#ifdef SCENE_XFORM_ACCESS
typealias SceneXFormsBufferType = BINDING_HANDLE(StructuredBuffer, SCENE_XFORM_ACCESS)<AffineXForm3Df>;
layout(SCENE_XFORM_BINDING + 0) restrict SceneXFormsBufferType SceneXForms;
layout(SCENE_XFORM_BINDING + 1) restrict SceneXFormsBufferType ScenePrevXForms;
#endif

#if BIND_SCENE_TLAS && CAN_BIND_TLAS
layout(SCENE_TLAS_BINDING + 0) uniform TLAS_t SceneTLAS;
#endif


struct BoundScene : Scene, IRayTracingScene
{

	Scene::UBO getUBO()
	{
		return SceneUBO;
	}

	Spectrumf<Mode> getAmbientSpectrum<int Mode>()
	{
		return Spectrumf<Mode>::MakeFromRGB(getUBO().ambient, RGBSourceType::Illuminant);
	}

#ifdef SCENE_MESH_ACCESS
	struct MeshReference : IMesh
	{
		typealias Vertex = Mesh::Vertex;
		// dynamic_uniform?
		dynamic_uniform uint index;

		__init(dynamic_uniform uint index)
		{
			this.index = index;
		}

		Mesh::Header getHeader()
		{
			return SceneMeshHeaders[index][0];
		}

		AABB3f getAABB()
		{
			return SceneMeshHeaders[index][0].box;
		}

		Mesh::Vertex readVertex(uint vertex_id)
		{
			return SceneMeshVertices[index][vertex_id];
		}

		template <int N>
		uvec<N> getPrimitiveIndices(uint primitive_id)
		{
			return Mesh::ReadIndices<N>(SceneMeshIndices[index], primitive_id, getHeader().flags);
		}

		template <int N>
		Vertex interpolateVertex(uvec<N> indices, Vectorf<N> weights)
		{
			return Mesh::InterpolateVertex(SceneMeshVertices[index], indices, weights);
		}

		template <int N>
		Vertex interpolateVertex(uvec<N> indices, Vectorf<N-1> uvs)
		{
			return interpolateVertex(indices, UVToBarycentric(uvs));
		}

		vec3 readTriangleNormal(uvec3 indices)
		{
			return Mesh::ReadTriangleNormal(SceneMeshVertices[index], indices);
		}

		SurfaceGeometryDifferentials<3> getTriangleDifferentials(
			const in Matrix3f object_to_world,
			const in uvec3 indices
		) {
			return Mesh::GetTriangleDifferentials(object_to_world, SceneMeshVertices[index], indices);
		}

		uint getVertexIndex(uint vertex_index, uint flags)
		{
			uint res = vertex_index;
			flags = flags & MESH_FLAG_INDEX_TYPE_MASK;
			if(flags == MESH_FLAG_INDEX_TYPE_UINT16)
			{
				res = SceneMeshIndices[index].Load<u16>(vertex_index * sizeof(u16));
			}
			else if(flags == MESH_FLAG_INDEX_TYPE_UINT32)
			{
				res = SceneMeshIndices[index].Load<u32>(vertex_index * sizeof(u32));
			}
			else if(flags == MESH_FLAG_INDEX_TYPE_UINT8)
			{
				res = SceneMeshIndices[index].Load<u8>(vertex_index * sizeof(u8));
			}
			return res;
		}
	};
#endif




__init()
{
	
}

#ifdef SCENE_MATERIAL_ACCESS
	typealias texture_id = u16;
	
	template <int N>
	Vector<texture_id, N> readMaterialTextureIDs(uint mid, uint base_slot = 0)
	{
		Vector<texture_id, N> res;
		const uint byte_offset = (mid * SCENE_TEXTURE_ID_PER_MATERIAL + base_slot) * sizeof(texture_id);
		res = SceneMaterialsTexturesIds.Load<Vector<texture_id, N>>(byte_offset);
		return res;
	}

	PBMaterial<Mode> readMaterial<int Mode>(uint material_id, vec2 uv, bool read_textures = true, ITextureSamplingInfo sampling_info = DefaultTextureSampling())
	{
		const PBMaterialProperties props = ScenePBMaterialsProps[material_id][0];
		PBMaterial<Mode> res = PBMaterial<Mode>(props);
		
		if(read_textures)
		{
			const uvec4 texture_ids = uvec4(readMaterialTextureIDs<4>(material_id));
			const uint albedo_texture_id = texture_ids[ALBEDO_ALPHA_TEXTURE_SLOT];
			const uint normal_texture_id = texture_ids[NORMAL_TEXTURE_SLOT];
			if((res.getFlags() & (MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT | MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT)) != 0 && albedo_texture_id != uint(-1))
			{
				const float4 aa = sampling_info.sample(SceneTextures2D[(albedo_texture_id)], uv);
				if((res.flags & MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT) != 0)
				{
					res.albedo = aa.xyz;
				}
				if((res.flags & MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT) != 0)
				{
					res.alpha = aa.w;
				}
			}
			if((res.flags & (MATERIAL_FLAG_USE_NORMAL_TEXTURE_BIT)) != 0 && normal_texture_id != uint(-1))
			{
				res.flags |= PB_MATERIAL_SAMPLED_NORMAL_TEXTURE;
				float3 n3 = sampling_info.sample(SceneTextures2D[(normal_texture_id)], uv).xyz;
				res.normal = Normalize(n3 * 2 - 1);
			}
		}
		return res;
	}

#ifdef SCENE_MESH_ACCESS

	bool TestTriangleOpacity<bool AssumeTexture=false>(uint object_index, uint primitive_index, vec2 triangle_uv)
	{
		bool res = false;
		const Scene::ObjectReference hit_object_ref = SceneObjectsTable[object_index];
		const MeshReference mesh = MeshReference(hit_object_ref.mesh_id);
		const Mesh::Header mesh_header = mesh.getHeader();
		const uvec3 vertices_id = mesh.getPrimitiveIndices<3>(primitive_index);
		const vec2 texture_uv = mesh.interpolateVertex(vertices_id, triangle_uv).uv;
		const uint material_id = hit_object_ref.material_id;
		const PBMaterialProperties props = ScenePBMaterialsProps[(material_id)][0];
		if(AssumeTexture || (props.flags & MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT) != 0)
		{
			const uint texture_id = readMaterialTextureIDs<1>(material_id, ALBEDO_ALPHA_TEXTURE_SLOT).x;
			if(AssumeTexture || texture_id != 0) // Assume is true
			{
				const float alpha = SceneTextures2D[(texture_id)].SampleLevel(texture_uv, 0).w;
				res = alpha >= getOpaqueAlphaThreshold();
			}
		}
		return res;
	}

#endif
#endif


#if defined(SCENE_MESH_ACCESS) && defined(SCENE_MATERIAL_ACCESS) && BIND_SCENE_TLAS && defined(SCENE_OBJECTS_ACCESS) && defined(SCENE_XFORM_ACCESS)

#if defined(SCENE_LIGHTS_ACCESS)

SkyLight<Mode> getSkyLight<int Mode>()
{
	let ubo = getUBO();
	SkyLight<Mode> res;

	// TODO compute atmosphere
	res.uniform_emission = Spectrumf<Mode>::MakeFromRGB(ubo.sky, RGBSourceType::Illuminant);
	// TODO black body spectral emission
	res.solar_disk_emission = Spectrumf<Mode>::MakeFromRGB(ubo.solar_disk_intensity, RGBSourceType::Illuminant);
	res.solar_disk_angle = ubo.solar_disk_radius;
	
	res.solar_disk_direction = ubo.solar_direction;
	res.solar_disk_cosine = ubo.solar_disk_cosine;
	
	return res;
}

static constexpr const uint LightIndexBits = 24;

bool sampleEnv()
{
	let ubo = getUBO();
	return NonZero(ubo.sky + ubo.solar_disk_intensity);
}

void spawnRayAtInfinity<RNG : IRNG>(inout RNG rng, inout Ray3f ray, inout float pdf)
{
	const float radius = getUBO().radius;
	let on_disk = SampleUniformOnDisk(rng.generate<float, 2>());
	mat3 basis = BasisFromDir(ray.direction);
	ray.origin = getUBO().center + radius * (basis * vec3(on_disk.value, 0) - ray.direction);
	pdf *= rcp(sqr(radius)) * on_disk.pdf;
}

float pdfSpawnAtInfinity(vec3 direction)
{
	return rcp(PI * sqr(getUBO().radius));
}

Light::EmissionSample<Mode> sampleLe<int Mode, RNG : IRNG>(inout RNG rng)
{
	// [Assume] would be better
	const uint n = getUBO().num_lights & BitMask<uint>(LightIndexBits);
	uint m = n;
	if(sampleEnv())
	{
		++m;
	}
	uint index = rng.generate<uint>(0, m);
	Light::EmissionSample<Mode> sample;
	if(index == n)
	{
		sample = getSkyLight<Mode>().sampleLe(rng);
	}
	else
	{
		sample = MakeLight<Mode>(SceneLights[index]).sampleLe(rng);
	}
	sample.index = index;
	sample.position_pdf *= rcp(float(m));
	// Spawn ray comming from infinity
	if(sample.flags & Light::Sample::Flags::AtInfinity)
	{
		spawnRayAtInfinity(rng, sample.ray, sample.position_pdf);
	}
	return sample;
}

RayPdf pdfLe<int Mode>(Ray3f ray, uint light_index)
{
	let ubo = getUBO();
	const uint n = ubo.num_lights & BitMask<uint>(LightIndexBits);
	uint m = n;
	if(sampleEnv())
	{
		++m;
	}
	float pmf_light = rcp(float(m));
	RayPdf res = {};
	if(light_index == n)
	{
		res = getSkyLight<Mode>().pdfLe(ray);
	}
	else
	{
		res = MakeLight<Mode>(SceneLights[light_index]).pdfLe(ray);
	}
	res.position_pdf *= pmf_light;
	bool at_infinity = light_index == n;
	if(at_infinity)
	{
		res.position_pdf *= pdfSpawnAtInfinity(ray.direction);
	}
	return res;
}

Light::ConnectionSample<Mode> sampleLi<int Mode, RNG : IRNG>(inout RNG rng, vec3 reference_position, const in Spectrumf<Mode> filter = Spectrumf<Mode>::One())
{
	let ubo = getUBO();
	const uint n = ubo.num_lights & BitMask<uint>(LightIndexBits);
	uint m = n;
	if(sampleEnv())
	{
		++m;
	}
	uint index = rng.generate<uint>(0, m);
	Light::ConnectionSample<Mode> sample;
	if(index == n)
	{
		sample = getSkyLight<Mode>().sampleLi(rng, reference_position, filter);
	}
	else
	{
		sample = MakeLight<Mode>(SceneLights[index]).sampleLi(rng, reference_position/*, filter*/);
	}
	
	sample.index = index;
	sample.pdf *= rcp(float(m));
	return sample;
}

float pdfLi<int Mode>(vec3 reference_position, vec3 direction, uint light_index, const in Spectrumf<Mode> filter = Spectrumf<Mode>::One())
{
	let ubo = getUBO();
	const uint n = ubo.num_lights & BitMask<uint>(LightIndexBits);
	uint m = n;
	if(sampleEnv())
	{
		++m;
	}
	float res = rcp(float(m));
	if(light_index == n)
	{
		res *= getSkyLight<Mode>().pdfLi(reference_position, direction, filter);
	}
	else
	{
		res *= MakeLight<Mode>(SceneLights[light_index]).pdfLi(reference_position, direction/*, filter*/);
	}
	return res;
}

float getEnvPdfLi<int Mode>(vec3 reference_position, vec3 direction, const in Spectrumf<Mode> filter = Spectrumf<Mode>::One())
{
	return pdfLi<Mode>(reference_position, direction, getUBO().num_lights & BitMask<uint>(LightIndexBits), filter);
}

RayPdf getEnvPdfLe<int Mode>(const in Ray3f ray)
{
	return pdfLe<Mode>(ray, getUBO().num_lights & BitMask<uint>(LightIndexBits));
}

Spectrumf<Mode> getEnvLe<int Mode>(vec3 direction)
{
	return getSkyLight<Mode>().Le(direction);
}

#endif

SurfaceShadingInfo reconstructTriangleSurfaceShadingInfo(
	uint object_index, uint primitive_index, vec2 barycentrics,
	AffineXForm3Df object_to_world
) {
	SurfaceShadingInfo res;

	const ObjectReference object_refs = SceneObjectsTable[object_index];
	const MeshReference mesh = MeshReference(object_refs.mesh_id);
	const uvec3 triangle_indices = mesh.getPrimitiveIndices<3>(primitive_index);
	const AffineXForm3Df xform = object_to_world;
	Mesh::Vertex interpolated_vertex = mesh.interpolateVertex(triangle_indices, barycentrics).transformedBy(xform);
	res.interpolated_normal = interpolated_vertex.normal;
	interpolated_vertex = interpolated_vertex.normalized();

	res.position = interpolated_vertex.position;
	res.geometry_normal = Normalize(DirectionMatrix(xform) * mesh.readTriangleNormal(triangle_indices));
	res.vertex_shading_normal = interpolated_vertex.normal;
	res.shading_normal = res.vertex_shading_normal;
	res.vertex_shading_tangent = interpolated_vertex.tangent;
	res.uv = interpolated_vertex.uv;
	
	return res;
}

#if SHADER_RAY_QUERY_AVAILABLE
	template <bool Committed = true, uint Flags>
	SurfaceShadingInfo reconstructTriangleSurfaceShadingInfo(inout RayQuery<Flags> rq)
	{
		constexpr const bool C = Committed;
		const uint object_index = rq.getInstanceID<C>();
		const uint primitive_index = rq.getPrimitiveIndex<C>();
		const vec2 barycentrics = rq.getTriangleBarycentrics<C>();
		const AffineXForm3Df xform = rq.getObjectToWorld3x4<C>();

		return reconstructTriangleSurfaceShadingInfo(
			object_index, primitive_index, barycentrics,
			xform
		);
	}
	
	
	// Only on candidate hit
	template <bool AssumeTexture, uint Flags>
	bool testTriangleCandidateHitOpacity(inout RayQuery<Flags> rq)
	{
		const uint object_index = rq.getRayInstanceCustomIndex<false>();
		const uint triangle_index = rq.getPrimitiveIndex<false>();
		return TestTriangleOpacity<AssumeTexture>(object_index, triangle_index, rq.getRayBarycentrics<false>());
	}

	template <uint Flags>
	void proceedRayQuery(inout RayQuery<Flags> rq)
	{
		while(rq.Proceed())
		{
			if(rq.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
			{
				if(testTriangleCandidateHitOpacity<true>(rq))
				{
					rq.CommitNonOpaqueTriangleHit();
				}
			}
		}
	}

	float traceVisibility(const in RayDesc ray)
	{
		float res = 1.0f;
		const uint ray_flags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
		//TraceRay(SceneTLAS, ray_flags, 0xff, 0, 1, )
		return res;
	}

#if SHADER_RAY_QUERY_AVAILABLE
	float queryVisibility(const in RayDesc ray)
	{
		float res = 1.0f;
		RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rq;
		rq.TraceRayInline(SceneTLAS, 0, ~0, ray);
		proceedRayQuery(rq);
		if(rq.CommittedStatus() == COMMITTED_NOTHING)
		{
			res = 1.0f;
		}
		else
		{
			if(rq.CommittedRayT() <= ray.TMax)
			{
				res = 0.0f;
			}
		}
		return res;
	}
#endif

	
#endif
	float lookupVisibility(const in RayDesc ray)
	{
		return 1.0f;
	}
	
	float testVisibility(const in RayDesc ray)
	{
#if SHADER_RAY_QUERY_AVAILABLE
		return queryVisibility(ray);
#else
		return 1;
#endif
	}
	
#endif
};


#endif

