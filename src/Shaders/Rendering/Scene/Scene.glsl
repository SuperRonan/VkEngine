#pragma once

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/Rendering/Lights/Light.glsl>

#include <ShaderLib:/Rendering/Mesh/MeshBinding.glsl>

#ifndef BIND_SCENE
#define BIND_SCENE 0
#endif


#if BIND_SCENE

#define SCENE_BINDING SCENE_DESCRIPTOR_BINDING

#ifndef LIGHTS_ACCESS
#define LIGHTS_ACCESS
#endif

layout(SCENE_BINDING + 0) uniform SceneUBOBinding
{
	vec3 ambient;
	uint num_lights;
} scene_ubo;

layout(SCENE_BINDING + 1) restrict LIGHTS_ACCESS buffer LightsBufferBinding
{
	Light lights[];
} lights_buffer;


// #ifndef SCENE_MAX_TEXTURE_BINDING
// #define SCENE_MAX_TEXTURE_BINDING
// #endif

#define SCENE_MESHS_BINDING SCENE_BINDING + 4

#ifndef SCENE_MESH_ACCESS 
#define SCENE_MESH_ACCESS
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

// layout(SCENE_BINDING + 4) uniform sampler2D SceneTextures[];
// layout(SCENE_BINDING + 5) uniform sampler2D SceneTextures2[];




#endif