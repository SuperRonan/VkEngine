#version 460

#include <ShaderLib:/Rendering/CubeMap.glsl>

#ifndef TARGET_CUBE
#define TARGET_CUBE 0
#endif

#if TARGET_CUBE
#extension GL_EXT_multiview : require
#endif

#define I_WANT_TO_DEBUG 0
#include <ShaderLib:/Debug/DebugBuffers.glsl>

#include "IndirectCommon.glsl"

// layout(location = 0) in vec3 a_position;
// layout(location = 1) in vec3 a_normal;
// layout(location = 2) in vec3 a_tangent;
// layout(location = 3) in vec2 a_uv;

// layout(location = 0) out vec3 v_w_position;
// layout(location = 0) out flat uvec4 v_flat;
// layout(location = 2) out vec2 v_uv;
// layout(location = 3) out vec3 v_w_normal;
// layout(location = 4) out vec3 v_w_tangent;

layout(push_constant) uniform pushConstant
{
	uint light_id;
} _pc;

void main()
{	
	const VertexData vd = fetchVertex();
	const Vertex vertex = vd.vertex;

	const vec3 a_position = vertex.position;
	// const vec3 a_normal = vertex.normal;
	// const vec3 a_tangent = vertex.tangent;
	// const vec2 a_uv = vertex.uv;

	const Light light = lights_buffer.lights[_pc.light_id];

#if TARGET_CUBE
	const mat4 w2p = cubeMapFaceProjection(gl_ViewIndex, light.position, POINT_LIGHT_DEFAULT_Z_NEAR);
#else
	const mat4 w2p = light.matrix;
#endif
	const mat4 o2w = mat4(vd.matrix);
	const mat4 o2p = w2p * o2w;
	
	vec3 m_position = a_position;
	
	gl_Position = o2p * vec4(m_position, 1);

	// const mat3 normal_matrix = mat3(o2w);

	// v_uv = a_uv;
	// // TODO Use the correct matrix (works as long as the scale is uniform)
	// v_w_normal = normal_matrix * a_normal;
	// v_w_tangent = normal_matrix * a_tangent;
	// v_w_position = (o2w * vec4(m_position, 1)).xyz;

	// v_flat.x = uint(gl_DrawID);
}