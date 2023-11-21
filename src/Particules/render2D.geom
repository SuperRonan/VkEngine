#version 460

#include "common.glsl"

#define I_WANT_TO_DEBUG 0
#include <ShaderLib:/debugBuffers.glsl>

layout(points) in;
layout(location = 0) in uint id[1];

layout(triangle_strip, max_vertices=4) out;
layout(location = 0) out flat uint v_type;
layout(location = 1) out vec2 v_uv;

layout(SHADER_DESCRIPTOR_BINDING + 0, std430) buffer readonly restrict b_state
{
	Particule particules[];
} state;

layout(push_constant) uniform PushConstants
{
	mat4 matrix;
	float zoom;
	uint num_particules;
} _pc;

void main()
{
	const Particule p = state.particules[id[0]];
	const float radius = p.radius;

	for(int i=0; i<2; ++i)
	{
		const float dx = float(i) - 0.5;
		for(int j=0; j<2; ++j)
		{
			const float dy = float(j) - 0.5;
			
			v_type = p.type;
			v_uv = vec2(dx, dy);
			gl_Position = _pc.matrix * vec4(p.position + vec2(dx, dy) * radius, 1, 1);
			EmitVertex();
		}
	}
	EndPrimitive();

#if I_WANT_TO_DEBUG
	
	{
		Caret c = Caret2D((_pc.matrix * vec4(p.position, 1, 1)).xy, 0);
		c = pushToDebugClipSpaceLn(id[0], c);
		c = pushToDebugClipSpaceLn(p.position, c);
		c = pushToDebugClipSpaceLn(p.velocity, c);
	}

#endif
}