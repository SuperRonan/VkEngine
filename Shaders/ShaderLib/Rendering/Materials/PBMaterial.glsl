#pragma once

#include "Material.glsl"

#include <ShaderLib:/random.glsl>

#include "PBMaterialDefinitions.h"

struct PBMaterialProperties
{
	vec3 albedo;
	uint flags;
	float metallic;
	float roughness;
	float cavity;
};

struct PBMaterialSampleData
{
	vec3 albedo;
	float alpha;
	vec3 normal;
	uint flags;
	float metallic;
	float roughness;
	float cavity;
};

PBMaterialProperties NoMaterialProps()
{
	PBMaterialProperties res;
	res.albedo = vec3(0, 0, 0);
	res.flags = 0;
	return res;
}


#if BIND_SINGLE_MATERIAL

layout(INVOCATION_DESCRIPTOR_BINDING + MATERIAL_BINDING_BASE + 0, std140) uniform restrict MaterialPropertiesBuffer
{
	PBMaterialProperties props;
} material_props;

layout(INVOCATION_DESCRIPTOR_BINDING + MATERIAL_BINDING_BASE + 1) uniform sampler2D AlbedoTexture;
layout(INVOCATION_DESCRIPTOR_BINDING + MATERIAL_BINDING_BASE + 2) uniform sampler2D NormalTexture;


#endif

vec3 getBoundMaterialAlbedo(vec2 uv)
{
#if BIND_SINGLE_MATERIAL
	const uint flags = material_props.props.flags;
	if((flags & MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT) != 0)
	{
		// See texture LOD	
		// const vec2 lod = textureQueryLod(AlbedoTexture, uv);
		// return RGBFromIndex(int(lod.y));
		return texture(AlbedoTexture, uv).rgb;
	}
	else
	{
		return material_props.props.albedo;
	}
#endif
	return 0..xxx;
}

#if BIND_SINGLE_MATERIAL
PBMaterialSampleData readBoundMaterial(vec2 uv, bool read_textures)
{
	const PBMaterialProperties props = material_props.props;
	PBMaterialSampleData res;
	res.flags = props.flags;
	res.albedo = 0..xxx;
	res.alpha = 1.0f;
	res.normal = vec3(0, 0, 1);
	if(read_textures && ((res.flags & (MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT | MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT)) != 0))
	{
		const vec4 albedo_alpha = texture(AlbedoTexture, uv);
		if((res.flags & MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT) != 0)
		{
			res.albedo = albedo_alpha.rgb;
		}
		if((res.flags & MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT) != 0)
		{
			res.alpha = albedo_alpha.a;
		}
	}
	else
	{
		res.albedo = props.albedo;
	}

	if(read_textures && ((res.flags & MATERIAL_FLAG_USE_NORMAL_TEXTURE_BIT) != 0))
	{
		res.normal = texture(NormalTexture, uv).xyz;
		res.normal = normalize(res.normal * 2 - 1);
	}

	res.metallic = props.metallic;
	res.roughness = props.roughness;
	res.cavity = props.cavity;

	return res;
}

PBMaterialSampleData readBoundMaterial(vec2 uv)
{
	return readBoundMaterial(uv, true);
}

#endif

