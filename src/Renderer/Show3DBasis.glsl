#version 460

#include <ShaderLib:/common.glsl>

#if SHADER_SEMANTIC_VERTEX

layout(location = 0) out uint vid;

void main()
{
	vid = gl_VertexIndex;
}

#endif


#if SHADER_SEMANTIC_GEOMETRY

layout(points) in;
layout(location = 0) in uint vid[1];

layout(line_strip, max_vertices=2) out;
layout(location = 0) out vec4 frag_color;

layout(push_constant) uniform PushConstant
{
	mat4 matrix;
} _pc;

void main()
{
	const uint axis = vid[0];

	const mat4 w2p = _pc.matrix;
	
	vec4 color = vec4(0..xxx, 1);
	color[axis] = 1;

	vec4 p0 = vec4(0..xxx, 1);
	vec4 p1 = p0;
	p1[axis] = 1;

	gl_Position = w2p * p0;
	frag_color = color;
	EmitVertex();

	gl_Position = w2p * p1;
	frag_color = color;
	EmitVertex();

	EndPrimitive();
}

#endif


#if SHADER_SEMANTIC_FRAGMENT

layout(location = 0) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main()
{
	out_color = in_color;
}

#endif