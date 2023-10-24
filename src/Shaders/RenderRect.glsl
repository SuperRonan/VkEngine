#version 460

#include <ShaderLib:/common.glsl>

layout(push_constant) uniform PushConstant
{
    vec2 pos;
    vec2 size;
    vec4 color;
} _pc;

#if SHADER_SEMANTIC_VERTEX


void main()
{
    const uint vid = gl_VertexIndex % 4;

    vec2 pos = 0..xx;
    if( vid >= 2)
    {
        pos.y = 1;
    }

    if(vid == 1 || vid == 2)
    {
        pos.x = 1;
    }

    pos = _pc.pos + pos * _pc.size;

    gl_Position = vec4(UVToClipSpace(pos), 0, 1);
}

#endif

#if SHADER_SEMANTIC_FRAGMENT

layout(location = 0) out vec4 o_color;

void main()
{
    o_color = _pc.color;
}

#endif