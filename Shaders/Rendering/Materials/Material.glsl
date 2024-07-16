#pragma once

#include <ShaderLib:/common.glsl>

#define MATERIAL_TYPE_NONE 0
#define MATERIAL_TYPE_PB 1


#ifndef MATERIAL_BINDING_BASE
#define MATERIAL_BINDING_BASE 8
#endif

#define MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT (1 << 1)
#define MATERIAL_FLAG_USE_NORMAL_TEXTURE_BIT (1 << 2)
