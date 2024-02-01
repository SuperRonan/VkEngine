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

struct ScenePBMaterial
{
	PBMaterialProperties props;
	uint albedo_texture_id;
	uint normal_texture_id;
	uint pad1;
	uint pad2;
};

#if BIND_SCENE

#extension GL_EXT_nonuniform_qualifier : require

#define SCENE_BINDING SCENE_DESCRIPTOR_BINDING + 0


layout(SCENE_BINDING + 0) uniform SceneUBOBinding
{
	uint num_lights;
	uint num_objects;
	uint num_mesh;
	uint num_materials;
	uint num_textures;
	
	vec3 ambient;
} scene_ubo;



#define SCENE_LIGHTS_BINDING SCENE_BINDING + 1
#ifndef LIGHTS_ACCESS
#define LIGHTS_ACCESS readonly
#endif

layout(SCENE_BINDING + 1) restrict LIGHTS_ACCESS buffer LightsBufferBinding
{
	Light lights[];
} lights_buffer;



#define SCENE_OBJECTS_BINDING SCENE_LIGHTS_BINDING + 1

#ifndef SCENE_OBJECTS_ACCESS
#define SCENE_OBJECTS_ACCESS readonly
#endif

layout(SCENE_OBJECTS_BINDING + 0) buffer restrict SCENE_OBJECTS_ACCESS SceneObjectsTable
{
	SceneObjectReference table[];
} scene_objects_table;


#define SCENE_MESHS_BINDING SCENE_OBJECTS_BINDING + 1

#ifndef SCENE_MESH_ACCESS 
#define SCENE_MESH_ACCESS readonly
#endif

layout(SCENE_MESHS_BINDING + 0) buffer restrict SCENE_MESH_ACCESS SceneMeshHeadersBindings
{
	MeshHeader headers;
} scene_mesh_headers[];

layout(SCENE_MESHS_BINDING + 1, std430) buffer restrict SCENE_MESH_ACCESS SceneMeshVerticesBindings
{
	Vertex vertices[];
} scene_mesh_vertices[];

// Vertex readSceneVertex(uint mesh_id, uint vertex_id)
// {
// 	const StorageVertex sv = scene_mesh_vertices[mesh_id].vertices[vertex_id];
// 	return MakeVertex(sv);
// } 

Vertex readSceneVertex(uint mesh_id, uint vertex_id)
{
	return scene_mesh_vertices[mesh_id].vertices[vertex_id];
} 

// Vertex readSceneVertex(uint mesh_id, uint vertex_id)
// {
// 	const uint float_per_vertex = 3 + 3 + 3 + 2;
// 	const uint vertex_data_base = vertex_id * float_per_vertex;
// 	Vertex res;
// 	res.position = vec3(
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 0],
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 1],
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 2]
// 	);
// 	res.normal = vec3(
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 3],
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 4],
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 5]
// 	);
// 	res.tangent = vec3(
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 6],
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 7],
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 8]
// 	);
// 	res.uv = vec2(
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 9],
// 		scene_mesh_vertices[mesh_id].data[vertex_data_base + 10]
// 	);
// 	return res;
// }

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


#define SCENE_MATERIAL_BINDING SCENE_MESHS_BINDING + 4

#ifndef SCENE_MATERIAL_ACCESS
#define SCENE_MATERIAL_ACCESS readonly
#endif

layout(SCENE_MATERIAL_BINDING + 0) buffer restrict SCENE_MATERIAL_ACCESS ScenePBMaterialsBinding
{
	ScenePBMaterial materials[];
} scene_pb_materals;


#define SCENE_TEXTURES_BINDING SCENE_MATERIAL_BINDING + 1

#ifndef SCENE_TEXTURE_ACCESS
#define SCENE_TEXTURE_ACCESS readonly
#endif

layout(SCENE_TEXTURES_BINDING + 0) uniform sampler2D SceneTextures2D[];


#define SCENE_XFORM_BINDING SCENE_TEXTURES_BINDING + 1

#ifndef SCENE_XFORM_ACCESS
#define SCENE_XFORM_ACCESS readonly
#endif

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