#pragma once

#include <ShaderLib:/common.glsl>

#include "MeshDefinitions.h"

struct MeshHeader
{
	uint num_vertices;
	uint num_indices;
	uint num_primitives;
	uint flags;
};

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
};

struct StorageVertex
{
	vec4 position;
	vec4 normal;
	vec4 tangent;
	vec4 uv;
};

Vertex MakeVertex(in const StorageVertex sv)
{
	Vertex res;
	res.position = sv.position.xyz;
	res.normal = sv.normal.xyz;
	res.tangent = sv.tangent.xyz;
	res.uv = sv.uv.xy;
	return res;
}



#if BIND_SINGLE_MESH

layout(MESH_BINDING_BASE + 0, std430) buffer restrict MESH_ACCESS MeshHeader
{
	MeshHeader header;
} bound_mesh_header;

layout(MESH_BINDING_BASE + 1, std430) buffer restrict MESH_ACCESS MeshVertices
{
	Vertex vertices[];
} bound_mesh_vertices;

layout(MESH_BINDING_BASE + 2, std430) buffer restrict MESH_ACCESS MeshIndices
{
	uint indices[];
} bound_mesh_indices32;


uint getBoundMeshIndex(uint i, uint flags)
{
	const uint index_type = flags & MESH_FLAG_INDEX_TYPE_MASK;
	uint res;
	if(index_type == MESH_FLAG_INDEX_TYPE_UINT16)
	{
		const uint chunk = bound_mesh_indices32.indices[i / 2];
		res = (chunk >> (16 * (i % 2))) & 0xffff; 
	}
	else if(index_type == MESH_FLAG_INDEX_TYPE_UINT32)
	{
		res = bound_mesh_indices32.indices[i];
	}
	else // if(index_type == MESH_FLAG_INDEX_TYPE_UINT8)
	{
		const uint chunk = bound_mesh_indices32.indices[i / 4];
		res = (chunk >> (8 * (i % 4))) & 0xff;
	}
	return res;
}

uvec3 getBoundMeshTriangleIndices(uint pid, uint flags)
{
	uvec3 res;
	const uint index_type = flags & MESH_FLAG_INDEX_TYPE_MASK;
	const uint i = pid * 3;
	if(index_type == MESH_FLAG_INDEX_TYPE_UINT32)
	{
		res = uvec3(bound_mesh_indices32.indices[i + 0],
					bound_mesh_indices32.indices[i + 1],
					bound_mesh_indices32.indices[i + 2]);
	}
	else
	{
		res = uvec3(getMeshIndex(i), getMeshIndex(i + 1), getMeshIndex(i + 2));
	}
	return res;
}

#endif