#version 450 

#include <ShaderLib:/common.glsl>

#if SHADER_SEMANTIC_VERTEX

layout(location = 0) in vec2 a_pos;

layout(location = 0) out vec4 v_color;

layout(push_constant) uniform PushConstant
{
    mat4 matrix;
    vec4 color;
} _pc;

void main()
{
    const vec2 w_pos = a_pos;
    gl_Position = (_pc.matrix * vec4(w_pos, 1, 1));
    v_color = _pc.color;
}

#endif


#if SHADER_SEMANTIC_FRAGMENT

layout(location = 0) in vec4 v_color;

layout(location = 0) out vec4 o_color;

void main()
{
    o_color = v_color;
}


#endif