#version 460

#include "common.glsl"

layout(points) in;
layout(location = 0) in uint id[1];

layout(triangle_strip, max_vertices=4) out;
layout(location = 0) out flat uint v_type;

layout(set = 0, binding = 0, std430) buffer readonly restrict b_state
{
	Particule particules[];
} state;

void main()
{
    const Particule p = state.particules[id[0]];

    for(int i=0; i<2; ++i)
    {
        const float dx = float(i) - 0.5;
        for(int j=0; j<2; ++j)
        {
            const float dy = float(j) - 0.5;
            
            v_type = p.type;
            gl_Position = vec4(p.position + vec2(dx, dy) * 0.01, 0, 1);
            EmitVertex();
        }
    }
    EndPrimitive();
}