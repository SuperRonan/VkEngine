#version 460

#include "common.glsl"

#define I_WANT_TO_DEBUG 0

#include <ShaderLib:/debugBuffers.glsl>
#include <ShaderLib:/random.glsl>

#include "IndirectCommon.glsl"

// layout(location = 0) in vec3 a_position;
// layout(location = 1) in vec3 a_normal;
// layout(location = 2) in vec3 a_tangent;
// layout(location = 3) in vec2 a_uv;

layout(location = 0) out vec3 v_w_position;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out vec3 v_w_normal;

void main()
{	
	const VertexData vd = fetchVertex();
	const Vertex vertex = vd.vertex;

	const vec3 a_position = vertex.position;
	const vec3 a_normal = vertex.normal;
	const vec3 a_tangent = vertex.tangent;
	const vec2 a_uv = vertex.uv;

	const mat4 w2p = ubo.world_to_proj;
	const mat4 o2w = mat4(vd.matrix);
	const mat4 o2p = w2p * o2w;
	
	
	gl_Position = o2p * vec4(a_position, 1);

	v_uv = a_uv;
	// TODO Use the correct matrix (works as long as the scale is uniform)
	v_w_normal = mat3(o2w) * a_normal;
	v_w_position = (o2w * vec4(a_position, 1)).xyz;

	{
		Caret crt = Caret3D(gl_Position, 0);

		crt = pushToDebugClipSpaceLn(gl_VertexIndex, crt);
	}
}