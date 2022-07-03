#version 460

#include "common.glsl"

layout(location = 0) in flat uint v_type;

layout(location = 0) out vec3 o_color;

void main()
{
    o_color = vec3(1, 0, 1);
}