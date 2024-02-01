#version 460


#include "common.glsl"

#define BIND_SCENE 1
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

#define I_WANT_TO_DEBUG 0
#include <ShaderLib:/DebugBuffers.glsl>

// layout(location = 0) in vec3 a_position;
// layout(location = 1) in vec3 a_normal;
// layout(location = 2) in vec3 a_tangent;
// layout(location = 3) in vec2 a_uv;

layout(location = 0) out vec3 v_w_position;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out vec3 v_w_normal;

layout(SHADER_DESCRIPTOR_BINDING + 1) buffer restrict readonly ModelIndexBuffer
{
	uint index[];
} model_indices;

void main()
{	
	const uint vid = uint(gl_VertexIndex);
	const uint did = uint(gl_DrawID);
	const uint model_id = model_indices.index[did];
	const SceneObjectReference model = scene_objects_table.table[model_id];
	const MeshHeader header = scene_mesh_headers[model.mesh_id].headers;
	const uint index = readSceneMeshIndex(model.mesh_id, vid, header.flags);

	// const Vertex vertex = scene_mesh_vertices[model.mesh_id].vertices[index];
	const Vertex vertex = readSceneVertex(model.mesh_id, index);

	const vec3 a_position = vertex.position;
	const vec3 a_normal = vertex.normal;
	const vec3 a_tangent = vertex.tangent;
	const vec2 a_uv = vertex.uv;

	const mat4 w2p = ubo.world_to_proj;
	const mat4 o2w = mat4(readSceneMatrix(model.xform_id));
	const mat4 o2p = w2p * o2w;
	
	vec3 m_position = a_position;
	
	gl_Position = o2p * vec4(m_position, 1);

	v_uv = a_uv;
	// TODO Use the correct matrix (works as long as the scale is uniform)
	v_w_normal = mat3(o2w) * a_normal;
	v_w_position = (o2w * vec4(m_position, 1)).xyz;

#if I_WANT_TO_DEBUG
	if(did == 0 && vid < 3)
	{
		DebugStringCaret c = Caret2D(vec2(0, 200 * vid), 0);
		c = pushToDebugPixLn(index, c); 
		c = pushToDebugPixLn(a_position, c);
	}
#endif
}