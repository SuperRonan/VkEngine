#pragma once

#include "Material.glsl"

struct PBMaterialProperties
{
	vec3 albedo;
	uint flags;
};

#ifndef BIND_SINGLE_MATERIAL
#define BIND_SINGLE_MATERIAL 0
#endif

#if BIND_SINGLE_MATERIAL

layout(INVOCATION_DESCRIPTOR_BINDING + MATERIAL_BINDING_BASE + 0, std140) uniform restrict MaterialPropertiesBuffer
{
	PBMaterialProperties props;
} material_props;

layout(INVOCATION_DESCRIPTOR_BINDING + MATERIAL_BINDING_BASE + 1) uniform sampler2D AlbedoTexture;


#endif

vec3 getBoundMaterialAlbedo(vec2 uv)
{
#if BIND_SINGLE_MATERIAL
	const uint flags = material_props.props.flags;
	if((flags & MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT) != 0)
	{
		return texture(AlbedoTexture, uv).rgb;
	}
	else
	{
		return material_props.props.albedo;
	}
#endif
	return 0..xxx;
}