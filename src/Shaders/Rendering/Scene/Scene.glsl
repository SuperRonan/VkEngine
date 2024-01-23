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


// #ifndef SCENE_MAX_TEXTURE_BINDING
// #define SCENE_MAX_TEXTURE_BINDING
// #endif

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

layout(SCENE_MESHS_BINDING + 1) buffer restrict SCENE_MESH_ACCESS SceneMeshVerticesBindings
{
	Vertex vertices[];
} scene_mesh_vertices[];

layout(SCENE_MESHS_BINDING + 2) buffer restrict SCENE_MESH_ACCESS SceneMeshIndicesBindings
{
	uint32_t indices[];
} scene_mesh_indices[];


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
	mat4x3 xforms[];
} scene_xforms;

layout(SCENE_XFORM_BINDING + 1) buffer restrict SCENE_XFORM_ACCESS ScenePrevXFormBinding
{
	mat4x3 xforms[];
} scene_prev_xforms;




#endif