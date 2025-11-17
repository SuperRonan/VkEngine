#pragma once

#include "CommonUBO.h"

#extension GL_EXT_scalar_block_layout : require

layout(COMMON_DESCRIPTOR_BINDING, std430) uniform CommonUBOBinding {CommonUBO _common_ubo;};
