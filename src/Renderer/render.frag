#version 460

#include "common.glsl"

layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 o_color;

void main()
{
    o_color = vec4(v_uv, 1.0 - (v_uv.x - v_uv.y), 0);
}