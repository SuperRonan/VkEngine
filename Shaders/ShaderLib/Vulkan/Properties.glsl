#pragma once

#include <ShaderLib:/core.glsl>

#define VK_DRIVER_ID_AMD_PROPRIETARY 1
#define VK_DRIVER_ID_AMD_OPEN_SOURCE 2
#define VK_DRIVER_ID_MESA_RADV 3
#define VK_DRIVER_ID_NVIDIA_PROPRIETARY 4
#define VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS 5
#define VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA 6
#define VK_DRIVER_ID_IMAGINATION_PROPRIETARY 7
#define VK_DRIVER_ID_QUALCOMM_PROPRIETARY 8
#define VK_DRIVER_ID_ARM_PROPRIETARY 9
#define VK_DRIVER_ID_GOOGLE_SWIFTSHADER 10
#define VK_DRIVER_ID_GGP_PROPRIETARY 11
#define VK_DRIVER_ID_BROADCOM_PROPRIETARY 12
#define VK_DRIVER_ID_MESA_LLVMPIPE 13
#define VK_DRIVER_ID_MOLTENVK 14
#define VK_DRIVER_ID_COREAVI_PROPRIETARY 15
#define VK_DRIVER_ID_JUICE_PROPRIETARY 16
#define VK_DRIVER_ID_VERISILICON_PROPRIETARY 17
#define VK_DRIVER_ID_MESA_TURNIP 18
#define VK_DRIVER_ID_MESA_V3DV 19
#define VK_DRIVER_ID_MESA_PANVK 20
#define VK_DRIVER_ID_SAMSUNG_PROPRIETARY 21
#define VK_DRIVER_ID_MESA_VENUS 22
#define VK_DRIVER_ID_MESA_DOZEN 23
#define VK_DRIVER_ID_MESA_NVK 24
#define VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA 25
#define VK_DRIVER_ID_MESA_AGXV 26



#define VK_SUBGROUP_FEATURE_BASIC_BIT 0x00000001
#define VK_SUBGROUP_FEATURE_VOTE_BIT 0x00000002
#define VK_SUBGROUP_FEATURE_ARITHMETIC_BIT 0x00000004
#define VK_SUBGROUP_FEATURE_BALLOT_BIT 0x00000008
#define VK_SUBGROUP_FEATURE_SHUFFLE_BIT 0x00000010
#define VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT 0x00000020
#define VK_SUBGROUP_FEATURE_CLUSTERED_BIT 0x00000040
#define VK_SUBGROUP_FEATURE_QUAD_BIT 0x00000080
#define VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV 0x00000100
#define VK_SUBGROUP_FEATURE_ROTATE_BIT_KHR 0x00000200
#define VK_SUBGROUP_FEATURE_ROTATE_CLUSTERED_BIT_KHR 0x00000400

#define VK_SHADER_STAGE_VERTEX_BIT 0x00000001
#define VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT 0x00000002
#define VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT 0x00000004
#define VK_SHADER_STAGE_GEOMETRY_BIT 0x00000008
#define VK_SHADER_STAGE_FRAGMENT_BIT 0x00000010
#define VK_SHADER_STAGE_COMPUTE_BIT 0x00000020
#define VK_SHADER_STAGE_ALL_GRAPHICS 0x0000001F
#define VK_SHADER_STAGE_ALL 0x7FFFFFFF
#define VK_SHADER_STAGE_RAYGEN_BIT_KHR 0x00000100
#define VK_SHADER_STAGE_ANY_HIT_BIT_KHR 0x00000200
#define VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR 0x00000400
#define VK_SHADER_STAGE_MISS_BIT_KHR 0x00000800
#define VK_SHADER_STAGE_INTERSECTION_BIT_KHR 0x00001000
#define VK_SHADER_STAGE_CALLABLE_BIT_KHR 0x00002000
#define VK_SHADER_STAGE_TASK_BIT_EXT 0x00000040
#define VK_SHADER_STAGE_MESH_BIT_EXT 0x00000080
#define VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI 0x00004000
#define VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI 0x00080000

#define VK_RAY_TRACING_INVOCATION_REORDER_MODE_NONE_NV 0
#define VK_RAY_TRACING_INVOCATION_REORDER_MODE_REORDER_NV 1


