#pragma once

#include <ShaderLib:/common.glsl>

#ifndef MATERIAL_BINDING_BASE
#define MATERIAL_BINDING_BASE 8
#endif

#define MATERIAL_TYPE_BITS 2
#define MATERIAL_TYPE_MASK BIT_MASK(MATERIAL_TYPE_BITS)

#define MATERIAL_TYPE_NONE 0
#define MATERIAL_TYPE_PB 1

#define MATERIAL_FLAG_TEXTURES_BIT_OFFSET MATERIAL_TYPE_BITS
#define MATERIAL_FLAG_TEXTURES_BITS 5
#define MATERIAL_FLAG_TEXTURES_MASK (BIT_MASK(MATERIAL_FLAG_TEXTURES_BITS) << MATERIAL_FLAG_TEXTURES_BIT_OFFSET)

#define MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT (1 << (MATERIAL_FLAG_TEXTURES_BIT_OFFSET + 0))
#define MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT (1 << (MATERIAL_FLAG_TEXTURES_BIT_OFFSET + 1))

#define MATERIAL_FLAG_USE_NORMAL_TEXTURE_BIT (1 << (MATERIAL_FLAG_TEXTURES_BIT_OFFSET + 2))

#define MATERIAL_FLAG_USE_ROUGHNESS_TEXTURE_BIT (1 << (MATERIAL_FLAG_TEXTURES_BIT_OFFSET + 3))
#define MATERIAL_FLAG_USE_METALLIC_TEXTURE_BIT (1 << (MATERIAL_FLAG_TEXTURES_BIT_OFFSET + 4))

#define MATERIAL_FLAG_HEMISPHERE_BIT_OFFSET (MATERIAL_FLAG_TEXTURES_BIT_OFFSET + MATERIAL_FLAG_TEXTURES_BITS)
#define MATERIAL_FLAG_REFLECTION_BIT (0x1 << MATERIAL_FLAG_HEMISPHERE_BIT_OFFSET) 
#define MATERIAL_FLAG_TRANSMISSION_BIT (0x2 << MATERIAL_FLAG_HEMISPHERE_BIT_OFFSET) 
