#version 460

#define BIND_RENDERER_SET 1

#include "common.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_uv;

layout(location = 0) out vec3 v_w_position;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out vec3 v_w_normal;
layout(location = 3) out vec3 v_w_tangent;

layout(push_constant) uniform PushConstant
{
	mat4 object_to_world;
} _pc;

void main()
{	
	const mat4 w2p = GetCameraWorldToProj(ubo.camera);
	const mat4 o2w = _pc.object_to_world;
	const mat4 o2p = w2p * o2w;
	
	vec3 m_position = a_position;
	
	gl_Position = o2p * vec4(m_position, 1);

	// TODO Use the correct matrix (works as long as the scale is uniform)
	const mat3 normal_matrix = mat3(o2w);

	v_uv = a_uv;
	v_w_normal = normal_matrix * a_normal;
	v_w_tangent = normal_matrix * a_tangent;
	v_w_position = (o2w * vec4(m_position, 1)).xyz;
}