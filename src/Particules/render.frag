#version 460

#include "common.glsl"

layout(location = 0) in flat uint v_type;
layout(location = 1) in vec2 v_uv;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 1) buffer readonly restrict ub_common_rules
{
	CommonRuleBuffer rules;
} ubo;

// layout(push_constant) uniform FragPushConstant
// {
//     layout(offset = 64) float zoom;
// };

void main()
{
    const float d = dot(v_uv, v_uv);
    if(d > 0.25)
    {
        discard;
        return;
    }
    else
    {
        o_color = ubo.rules.particules_properties[v_type].color;

        if(d > 0.20)
        {
            o_color.xyz *= 0.5;
        }
    }
}