#pragma once

#include "common.glsl"

#define BIND_SCENE 1
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

layout(SHADER_DESCRIPTOR_BINDING + 0) buffer restrict readonly ModelIndexBuffer
{
	uint index[];
} model_indices;

#if SHADER_SEMANTIC_VERTEX


struct VertexData
{
    Vertex vertex;
    mat4x3 matrix;
};

VertexData fetchVertex()
{
    const uint vid = uint(gl_VertexIndex);
	const uint did = uint(gl_DrawID);
	const uint model_id = model_indices.index[did];
	const SceneObjectReference model = scene_objects_table.table[model_id];
	const MeshHeader header = scene_mesh_headers[model.mesh_id].headers;
	const uint index = readSceneMeshIndex(model.mesh_id, vid, header.flags);
    VertexData res;
	res.vertex = readSceneVertex(model.mesh_id, index);
    res.matrix = readSceneMatrix(model.xform_id);
    return res;
}

#endif