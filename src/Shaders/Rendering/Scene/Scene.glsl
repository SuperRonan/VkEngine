#pragma once

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/Rendering/Lights/Light.glsl>

#include <ShaderLib:/Rendering/Mesh/MeshBinding.glsl>

#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

#ifndef BIND_SCENE
#define BIND_SCENE 0
#endif

#define SCENE_OBJECT_FLAG_VISIBLE_BIT 1


struct SceneObjectReference
{
	uint mesh_id;
	uint material_id;
	uint xform_id;
	uint flags;
};

struct ScenePBMaterialTextures
{
	uint albedo_texture_id;
	uint normal_texture_id;
	uint pad1;
	uint pad2;
};

ScenePBMaterialTextures NoPBMaterialTextures()
{
	ScenePBMaterialTextures res;
	res.albedo_texture_id = -1;
	res.normal_texture_id = -1;
	return res;
}

#if BIND_SCENE

#extension GL_EXT_nonuniform_qualifier : require

#define SCENE_BINDING SCENE_DESCRIPTOR_BINDING + 0

#define SCENE_LIGHTS_BINDING SCENE_BINDING + 1
#define SCENE_LIGHTS_NUM_BINDING 1
#ifndef LIGHTS_ACCESS
#define LIGHTS_ACCESS readonly
#endif

#define SCENE_OBJECTS_BINDING SCENE_LIGHTS_BINDING + SCENE_LIGHTS_NUM_BINDING
#define SCENE_OBJECTS_NUM_BINDING 1
#ifndef SCENE_OBJECTS_ACCESS
#define SCENE_OBJECTS_ACCESS readonly
#endif

#define SCENE_MESHS_BINDING SCENE_OBJECTS_BINDING + SCENE_OBJECTS_NUM_BINDING
#define SCENE_MESHS_NUM_BINDING 3
#ifndef SCENE_MESH_ACCESS 
#define SCENE_MESH_ACCESS readonly
#endif

#define SCENE_MATERIAL_BINDING SCENE_MESHS_BINDING + SCENE_MESHS_NUM_BINDING
#define SCENE_MATERIAL_NUM_BINDING 2
#ifndef SCENE_MATERIAL_ACCESS
#define SCENE_MATERIAL_ACCESS readonly
#endif

#define SCENE_TEXTURES_BINDING SCENE_MATERIAL_BINDING + SCENE_MATERIAL_NUM_BINDING
#define SCENE_TEXTURES_NUM_BINDING 1
#ifndef SCENE_TEXTURE_ACCESS
#define SCENE_TEXTURE_ACCESS readonly
#endif

#define SCENE_XFORM_BINDING SCENE_TEXTURES_BINDING + SCENE_TEXTURES_NUM_BINDING
#define SCENE_XFORM_NUM_BINDING 2
#ifndef SCENE_XFORM_ACCESS
#define SCENE_XFORM_ACCESS readonly
#endif


layout(SCENE_BINDING + 0) uniform SceneUBOBinding
{
	uint num_lights;
	uint num_objects;
	uint num_mesh;
	uint num_materials;
	uint num_textures;
	
	vec3 ambient;
} scene_ubo;




layout(SCENE_BINDING + 1) restrict LIGHTS_ACCESS buffer LightsBufferBinding
{
	Light lights[];
} lights_buffer;




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
	return scene_mesh_vertices[mesh_id].vertices[vertex_id];
}

Vertex interpolateSceneVertex(uint mesh_id, uvec3 vertex_ids, vec2 triangle_uv)
{
	const vec3 bary = triangleUVToBarycentric(triangle_uv);
	Vertex res;
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




layout(SCENE_MATERIAL_BINDING + 0) buffer restrict SCENE_MATERIAL_ACCESS ScenePBMaterialsBinding
{
	PBMaterialProperties props;
} scene_pb_materials[];

layout(SCENE_MATERIAL_BINDING + 1) buffer restrict SCENE_MATERIAL_ACCESS ScenePBMaterialsRefBinding
{
	ScenePBMaterialTextures ids[];
} scene_pb_materials_textures;




layout(SCENE_TEXTURES_BINDING + 0) uniform sampler2D SceneTextures2D[];




layout(SCENE_XFORM_BINDING + 0) buffer restrict SCENE_XFORM_ACCESS SceneXFormBinding
{
	mat3x4 xforms[];
} scene_xforms;

mat4x3 readSceneMatrix(uint id)
{
	const mat4x3 res = transpose(scene_xforms.xforms[id]);
	return res;
}

layout(SCENE_XFORM_BINDING + 1) buffer restrict SCENE_XFORM_ACCESS ScenePrevXFormBinding
{
	mat4x3 xforms[];
} scene_prev_xforms;




#endif