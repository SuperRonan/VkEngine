#pragma once

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/Rendering/Lights/Light.glsl>

#include <ShaderLib:/Rendering/Geometry/MeshBinding.glsl>

#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

#include <ShaderLib:/RayTracingCommon.glsl>

#ifndef BIND_ALL_SCENE_COMPONENTS
#define BIND_ALL_SCENE_COMPONENTS 1
#endif

#include "SceneDefinitions.h"



struct SceneObjectReference
{
	uint mesh_id;
	uint material_id;
	uint xform_id;
	uint flags;
};

struct MaterialTextureIds
{
	uint16_t ids[SCENE_TEXTURE_ID_PER_MATERIAL];
};

MaterialTextureIds NoMaterialTextureIds()
{
	MaterialTextureIds res;
	for(uint i = 0; i < 8; ++i)
	{
		res.ids[i] = uint16_t(-1);
	}
	return res;
}

uint GetMaterialTextureId(const in MaterialTextureIds m, uint slot)
{
	return uint(m.ids[slot]);
}

#if BIND_SCENE

#extension GL_EXT_nonuniform_qualifier : require


layout(SCENE_BINDING + 0) uniform SceneUBOBinding
{
	uint num_lights;
	uint num_objects;
	uint num_mesh;
	uint num_materials;
	uint num_textures;
	
	vec3 ambient;
	vec3 sky;
} scene_ubo;

float GetSceneOpaqueAlphaThreshold()
{
	return 0.1f;
}


layout(SCENE_LIGHTS_BINDING + 0) restrict SCENE_LIGHTS_ACCESS buffer LightsBufferBinding
{
	Light lights[];
} lights_buffer;

layout(SCENE_LIGHTS_BINDING + 1) uniform texture2D LightsDepth2D[];
layout(SCENE_LIGHTS_BINDING + 2) uniform textureCube LightsDepthCube[];



layout(SCENE_OBJECTS_BINDING + 0) buffer restrict SCENE_OBJECTS_ACCESS SceneObjectsTable
{
	SceneObjectReference table[];
} scene_objects_table;



layout(SCENE_MESHS_BINDING + 0) buffer restrict SCENE_MESH_ACCESS SceneMeshHeadersBindings
{
	MeshHeader headers;
} scene_mesh_headers[];

layout(SCENE_MESHS_BINDING + 1, std430) buffer restrict SCENE_MESH_ACCESS SceneMeshVerticesBindings
{
	Vertex vertices[];
} scene_mesh_vertices[];

Vertex readSceneVertex(uint mesh_id, uint vertex_id)
{
	return scene_mesh_vertices[NonUniformEXT(mesh_id)].vertices[vertex_id];
}

Vertex interpolateSceneVertex(uint mesh_id, uvec3 vertex_ids, vec2 triangle_uv)
{
	const vec3 bary = triangleUVToBarycentric(triangle_uv);
	Vertex res;
	res.position = 0..xxx;
	res.normal = 0..xxx;
	res.tangent = 0..xxx;
	res.uv = 0..xx;
	for(uint i=0; i<3; ++i)
	{
		const Vertex src = readSceneVertex(mesh_id, vertex_ids[i]);
		const float w = bary[i];
		res.position += src.position * w;
		res.normal += src.normal * w;
		res.tangent += src.tangent * w;
		res.uv += src.uv * w;
	}
	return res;
}

Vertex interpolateSceneVertexAndNormalize(uint mesh_id, uvec3 vertex_ids, vec2 triangle_uv)
{
	Vertex res = interpolateSceneVertex(mesh_id, vertex_ids, triangle_uv);
	res.normal = normalize(res.normal);
	res.tangent = normalize(res.tangent);
	return res;
}

void readSceneTriangleVertexPositions(uint mesh_id, uvec3 vertex_ids, out vec3 res[3])
{
	for(uint i=0; i < 3; ++i)
	{
		res[i] = vec3(scene_mesh_vertices[NonUniformEXT(mesh_id)].vertices[vertex_ids[i]].position);
	}
}

mat3 readSceneTriangleVertexPositions(uint mesh_id, uvec3 vertex_ids)
{
	mat3 res;
	for(uint i=0; i < 3; ++i)
	{
		res[i] = vec3(scene_mesh_vertices[NonUniformEXT(mesh_id)].vertices[vertex_ids[i]].position);
	}
	return res;
}

layout(SCENE_MESHS_BINDING + 2) buffer restrict SCENE_MESH_ACCESS SceneMeshIndicesBindings
{
	uint32_t indices[];
} scene_mesh_indices[];

uint readSceneMeshIndex(uint mesh_id, uint index_id, uint flags)
{
	uint res = 0;
	const uint index_type = flags & MESH_FLAG_INDEX_TYPE_MASK;
	if(index_type == MESH_FLAG_INDEX_TYPE_UINT16)
	{
		const uint chunk = scene_mesh_indices[mesh_id].indices[index_id / 2];
		res = (chunk >> (16 * (index_id % 2))) & 0xffff; 
	}
	else if(index_type == MESH_FLAG_INDEX_TYPE_UINT32)
	{
		res = scene_mesh_indices[mesh_id].indices[index_id];
	}
	else // if(index_type == MESH_FLAG_INDEX_TYPE_UINT8)
	{
		const uint chunk = scene_mesh_indices[mesh_id].indices[index_id/ 4];
		res = (chunk >> (8 * (index_id % 4))) & 0xff;
	}
	return res;
}

uvec3 getSceneMeshTriangleVerticexIndices(uint mesh_id, uint primitive_id, uint flags)
{
	uvec3 res = uvec3(0);
	res.x = readSceneMeshIndex(mesh_id, primitive_id * 3 + 0, flags);
	res.y = readSceneMeshIndex(mesh_id, primitive_id * 3 + 1, flags);
	res.z = readSceneMeshIndex(mesh_id, primitive_id * 3 + 2, flags);
	return res;
}


layout(SCENE_MATERIAL_BINDING + 0) buffer restrict SCENE_MATERIAL_ACCESS ScenePBMaterialsBinding
{
	PBMaterialProperties props;
} scene_pb_materials[];

layout(SCENE_MATERIAL_BINDING + 1) buffer restrict SCENE_MATERIAL_ACCESS ScenePBMaterialsRefBinding
{
	MaterialTextureIds ids[];
} scene_pb_materials_textures;




layout(SCENE_TEXTURES_BINDING + 0) uniform sampler2D SceneTextures2D[];




layout(SCENE_XFORM_BINDING + 0) buffer restrict SCENE_XFORM_ACCESS SceneXFormBinding
{
	mat4x3 xforms[];
} scene_xforms;

mat4x3 readSceneMatrix(uint id)
{
	const mat4x3 res = (scene_xforms.xforms[id]);
	return res;
}

layout(SCENE_XFORM_BINDING + 1) buffer restrict SCENE_XFORM_ACCESS ScenePrevXFormBinding
{
	mat4x3 xforms[];
} scene_prev_xforms;


// Returns true if Opaque
bool SceneTestTriangleOpacity(uint object_index, uint primitive_id, vec2 triangle_uv)
{
	bool res = false;
	const SceneObjectReference hit_object_ref = scene_objects_table.table[object_index];
	const MeshHeader mesh_header = scene_mesh_headers[NonUniformEXT(hit_object_ref.mesh_id)].headers;
	const uvec3 vertices_id = getSceneMeshTriangleVerticexIndices(hit_object_ref.mesh_id, primitive_id, mesh_header.flags);
	const vec2 texture_uv = interpolateSceneVertex(hit_object_ref.mesh_id, vertices_id, triangle_uv).uv;
	
	const uint material_id = hit_object_ref.material_id;
	const PBMaterialProperties props = scene_pb_materials[NonUniformEXT(material_id)].props;

	//if((props.flags & MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT) != 0) // assumed to be true since not opaque
	{
		const uint tex_id = GetMaterialTextureId(scene_pb_materials_textures.ids[material_id], ALBEDO_ALPHA_TEXTURE_SLOT);
		// TextureLod for now, then deduce LOD from ray derivatives
		float alpha = textureLod(SceneTextures2D[NonUniformEXT(tex_id)], texture_uv, 0).w;
		res = alpha >= GetSceneOpaqueAlphaThreshold();
	}
	return res;
}

bool SceneTestTriangleOpacityIFN(uint object_index, uint primitive_id, vec2 triangle_uv)
{
	bool res = false;
	const SceneObjectReference hit_object_ref = scene_objects_table.table[object_index];
	const uint material_id = hit_object_ref.material_id;
	const PBMaterialProperties props = scene_pb_materials[NonUniformEXT(material_id)].props;
	if((props.flags & MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT) != 0)
	{
		res = SceneTestTriangleOpacity(object_index, primitive_id, triangle_uv);
	}
	return res;
}


#if CAN_BIND_TLAS
layout(SCENE_TLAS_BINDING + 0) uniform TLAS_t SceneTLAS;

#if SHADER_RAY_QUERY_AVAILABLE

#endif // SHADER_RAY_QUERY_AVAILABLE

#endif // CAN_BIND_TLAS



#endif // BIND_SCENE