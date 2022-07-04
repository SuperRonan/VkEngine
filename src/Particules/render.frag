#version 460

#include "common.glsl"

layout(location = 0) in flat uint v_type;
layout(location = 1) in vec2 v_uv;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 1) uniform ub_common_rules
{
	CommonRuleBuffer rules;
} ubo;

void main()
{
    if(dot(v_uv, v_uv) > 0.25)
    {
        discard;
        return;
    }
    else
    {
        o_color = ubo.rules.particules_properties[v_type].color;
    }
}