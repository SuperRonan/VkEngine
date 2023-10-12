#version 460

#include "common.glsl"

#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_w_normal;

layout(location = 0) out vec4 o_color;

void main()
{
    const PBMaterialProperties props = material_props.props;
    
    vec3 res = 0..xxx;

    res += props.albedo;

    o_color = vec4(res, 1);
}