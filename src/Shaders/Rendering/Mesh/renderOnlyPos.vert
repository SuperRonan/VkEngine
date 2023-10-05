#version 450 

#include <ShaderLib:/common.glsl>

#ifndef DIMENSIONS
#error "Unknown dimensions"
#endif

#if DIMENSIONS == 2
#define vecD vec2
#elif DIMENSIONS == 3
#define vecD vec3
#endif

layout(location = 0) in vecD a_pos;

layout(location = 0) out vec4 v_color;

layout(push_constant) uniform PushConstant
{
    mat4 matrix;
    vec4 color;
} _pc;

void main()
{
    const vecD w_pos = a_pos;
#if DIMENSIONS == 2
    const vec4 h_pos = vec4(w_pos, 1, 1);
#elif DIMENSIONS == 3
    const vec4 h_pos = vec4(w_pos, 1);
#endif
    gl_Position = (_pc.matrix * h_pos);
    v_color = _pc.color;
}

