#version 460

#include "common.glsl"
#include "../Shaders/random.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

layout(location = 0) in vec2 v_uv[3];
layout(location = 1) in vec3 v_p_position[3];

layout(location = 0) out vec2 o_uv;

void main()
{
	gl_Position = vec4(v_p_position[0], 1);
	o_uv = v_uv[0];
	EmitVertex();

	gl_Position = vec4(v_p_position[1], 1);
	o_uv = v_uv[1];
	EmitVertex();

	gl_Position = vec4(v_p_position[2], 1);
	o_uv = v_uv[2];
	EmitVertex();

	EndPrimitive();
}