#pragma once

#include "common.slang"

#define BIND_SCENE 1
#include <ShaderLib/Rendering/Scene/Scene.slang>


layout(SHADER_DESCRIPTOR_BINDING + 0) StructuredBuffer<uint> indirect_model_indices;

#if SHADER_SEMANTIC_VERTEX


struct VertexData
{
	Mesh::Vertex vertex;
	AffineXForm3Df matrix;
};

VertexData FetchIndirectVertex(const in BoundScene scene, uint draw_id, uint vertex_id)
{
	VertexData res;
	const uint model_id = indirect_model_indices[draw_id];
	let object_ref = SceneObjectsTable[model_id];
	let mesh = BoundScene::MeshReference(object_ref.mesh_id);
	const uint index = mesh.getVertexIndex(vertex_id, mesh.getHeader().flags);
	res.vertex = mesh.readVertex(index);
	res.matrix = SceneXForms[object_ref.xform_id];
	return res;
}

#endif