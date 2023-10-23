#version 460

#include "common.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_uv;

layout(location = 0) out vec3 v_w_position;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out vec3 v_w_normal;

layout(push_constant) uniform PushConstant
{
	mat4 object_to_world;
} _pc;

void main()
{	
	const mat4 w2p = ubo.world_to_proj;
	const mat4 o2w = _pc.object_to_world;
	const mat4 o2p = w2p * o2w;
	
	
	gl_Position = o2p * vec4(a_position, 1);

	v_uv = a_uv;
	// TODO Use the correct matrix (works as long as the scale is uniform)
	v_w_normal = mat3(o2w) * a_normal;
	v_w_position = (o2w * vec4(a_position, 1)).xyz;
}