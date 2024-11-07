#version 460

#define BIND_SCENE 1
#define BIND_RENDERER_SET 1

#include "common.glsl"


#define I_WANT_TO_DEBUG 0
#include <ShaderLib:/Debug/debugBuffers.glsl>

#include <ShaderLib:/random.glsl>

#include "IndirectCommon.glsl"

#include <ShaderLib:/Maths/transforms.glsl>

// layout(location = 0) in vec3 a_position;
// layout(location = 1) in vec3 a_normal;
// layout(location = 2) in vec3 a_tangent;
// layout(location = 3) in vec2 a_uv;

layout(location = 0) out flat uvec4 v_flat;
layout(location = 1) out vec3 v_w_position;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec3 v_w_normal;
layout(location = 4) out vec3 v_w_tangent;

void main()
{	
	const VertexData vd = fetchVertex();
	const Vertex vertex = vd.vertex;

	const vec3 a_position = vertex.position;
	const vec3 a_normal = vertex.normal;
	const vec3 a_tangent = vertex.tangent;
	const vec2 a_uv = vertex.uv;

	const mat4 w2p = GetCameraWorldToProj(ubo.camera);
	const mat4x3 o2w = vd.matrix;
	const mat4 o2p = w2p * mat4(o2w);
	
	
	gl_Position = o2p * vec4(a_position, 1);

	const mat3 normal_matrix = DirectionMatrix(mat3(o2w));

	v_uv = a_uv;
	v_w_normal = normalize(normal_matrix * a_normal);
	v_w_tangent = normalize(normal_matrix * a_tangent);
	v_w_position = (o2w * vec4(a_position, 1)).xyz;

	v_flat.x = gl_DrawID;
	
}