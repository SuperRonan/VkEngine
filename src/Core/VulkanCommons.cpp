#include "VulkanCommons.hpp"


namespace vkl
{
	using namespace std::string_literals;

	VulkanFeatures filterFeatures(VulkanFeatures const& requested, VulkanFeatures const& available)
	{
		const auto op_and = [](VkBool32 a, VkBool32 b) {return a & b; };

		VulkanFeatures res;

		VkBool32ArrayOp(
			&res.features.robustBufferAccess,
			&requested.features.robustBufferAccess,
			&available.features.robustBufferAccess,
			(offsetof(VkPhysicalDeviceFeatures, inheritedQueries) - offsetof(VkPhysicalDeviceFeatures, robustBufferAccess)) / sizeof(VkBool32) + 1,
			op_and
		);

#define FILTER_VK_FEATURES(VK_VER, firstFeature, lastFeature) \
		VkBool32ArrayOp( \
			&res.features_##VK_VER . firstFeature, \
			&requested.features_##VK_VER . firstFeature, \
			&available.features_##VK_VER . firstFeature, \
			(offsetof(VkPhysicalDeviceVulkan##VK_VER##Features, lastFeature) - offsetof(VkPhysicalDeviceVulkan##VK_VER##Features, firstFeature)) / sizeof(VkBool32) + 1, \
			op_and \
		)

		FILTER_VK_FEATURES(11, storageBuffer16BitAccess, shaderDrawParameters);
		FILTER_VK_FEATURES(12, samplerMirrorClampToEdge, subgroupBroadcastDynamicId);
		FILTER_VK_FEATURES(13, robustImageAccess, maintenance4);

#undef FILTER_VK_FEATURES

		VkBool32ArrayOp(
			&res.line_raster_ext.rectangularLines,
			&requested.line_raster_ext.rectangularLines,
			&available.line_raster_ext.rectangularLines,
			(offsetof(VkPhysicalDeviceLineRasterizationFeaturesEXT, stippledSmoothLines) - offsetof(VkPhysicalDeviceLineRasterizationFeaturesEXT, rectangularLines)) / sizeof(VkBool32) + 1,
			op_and
		);

		res.index_uint8_ext.indexTypeUint8 = requested.index_uint8_ext.indexTypeUint8 & available.index_uint8_ext.indexTypeUint8;

		VkBool32ArrayOp(
			&res.mesh_shader_ext.taskShader,
			&requested.mesh_shader_ext.taskShader,
			&available.mesh_shader_ext.taskShader,
			(offsetof(VkPhysicalDeviceMeshShaderFeaturesEXT, meshShaderQueries) - offsetof(VkPhysicalDeviceMeshShaderFeaturesEXT, taskShader)) / sizeof(VkBool32) + 1,
			op_and
		);

		return res;
	}



	std::string getVkPresentModeKHRName(VkPresentModeKHR p)
	{
		std::string res;
		if (p == VK_PRESENT_MODE_IMMEDIATE_KHR)						res = "Immediate";
		if (p == VK_PRESENT_MODE_MAILBOX_KHR)						res = "Mailbox";
		if (p == VK_PRESENT_MODE_FIFO_KHR)							res = "FIFO";
		if (p == VK_PRESENT_MODE_FIFO_RELAXED_KHR)					res = "FIFO Relaxed";
		if (p == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR)			res = "Shared Demand Refresh";
		if (p == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR)		res = "Shader Continuous Refresh";
		return res;
	}

	std::string getVkColorSpaceKHRName(VkColorSpaceKHR c)
	{
		std::string res;
		if(c == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)					res = "sRGB NonLinear";
		if(c == VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT)			res = "Display P3 NonLinear";
		if(c == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)			res = "Extended sRGB Linear";
		if(c == VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT)				res = "Display P3 Linear";
		if(c == VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT)				res = "DCI P3 NonLinear";
		if(c == VK_COLOR_SPACE_BT709_LINEAR_EXT)					res = "BT709 Linear";
		if(c == VK_COLOR_SPACE_BT709_NONLINEAR_EXT)					res = "BT709 NonLinear";
		if(c == VK_COLOR_SPACE_BT2020_LINEAR_EXT)					res = "BT2020 Linear";
		if(c == VK_COLOR_SPACE_HDR10_ST2084_EXT)					res = "HDR10 ST2084";
		if(c == VK_COLOR_SPACE_DOLBYVISION_EXT)						res = "Dolby Vision";
		if(c == VK_COLOR_SPACE_HDR10_HLG_EXT)						res = "HDR10 HLG";
		if(c == VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT)					res = "Adobe RGB Linear";
		if(c == VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT)				res = "Adobe RGB NonLinear";
		if(c == VK_COLOR_SPACE_PASS_THROUGH_EXT)					res = "Pass Through";
		if(c == VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT)			res = "Extended sRGB NonLinear";
		if(c == VK_COLOR_SPACE_DISPLAY_NATIVE_AMD)					res = "Display Native";
		return res;
	}

	static const std::vector<const char*> vk_format_names = {
		"UNDEFINED",
		"R4G4_UNORM_PACK8",
		"R4G4B4A4_UNORM_PACK16",
		"B4G4R4A4_UNORM_PACK16",
		"R5G6B5_UNORM_PACK16",
		"B5G6R5_UNORM_PACK16",
		"R5G5B5A1_UNORM_PACK16",
		"B5G5R5A1_UNORM_PACK16",
		"A1R5G5B5_UNORM_PACK16",
		"R8_UNORM",
		"R8_SNORM",
		"R8_USCALED",
		"R8_SSCALED",
		"R8_UINT",
		"R8_SINT",
		"R8_SRGB",
		"R8G8_UNORM",
		"R8G8_SNORM",
		"R8G8_USCALED",
		"R8G8_SSCALED",
		"R8G8_UINT",
		"R8G8_SINT",
		"R8G8_SRGB",
		"R8G8B8_UNORM",
		"R8G8B8_SNORM",
		"R8G8B8_USCALED",
		"R8G8B8_SSCALED",
		"R8G8B8_UINT",
		"R8G8B8_SINT",
		"R8G8B8_SRGB",
		"B8G8R8_UNORM",
		"B8G8R8_SNORM",
		"B8G8R8_USCALED",
		"B8G8R8_SSCALED",
		"B8G8R8_UINT",
		"B8G8R8_SINT",
		"B8G8R8_SRGB",
		"R8G8B8A8_UNORM",
		"R8G8B8A8_SNORM",
		"R8G8B8A8_USCALED",
		"R8G8B8A8_SSCALED",
		"R8G8B8A8_UINT",
		"R8G8B8A8_SINT",
		"R8G8B8A8_SRGB",
		"B8G8R8A8_UNORM",
		"B8G8R8A8_SNORM",
		"B8G8R8A8_USCALED",
		"B8G8R8A8_SSCALED",
		"B8G8R8A8_UINT",
		"B8G8R8A8_SINT",
		"B8G8R8A8_SRGB",
		"A8B8G8R8_UNORM_PACK32",
		"A8B8G8R8_SNORM_PACK32",
		"A8B8G8R8_USCALED_PACK32",
		"A8B8G8R8_SSCALED_PACK32",
		"A8B8G8R8_UINT_PACK32",
		"A8B8G8R8_SINT_PACK32",
		"A8B8G8R8_SRGB_PACK32",
		"A2R10G10B10_UNORM_PACK32",
		"A2R10G10B10_SNORM_PACK32",
		"A2R10G10B10_USCALED_PACK32",
		"A2R10G10B10_SSCALED_PACK32",
		"A2R10G10B10_UINT_PACK32",
		"A2R10G10B10_SINT_PACK32",
		"A2B10G10R10_UNORM_PACK32",
		"A2B10G10R10_SNORM_PACK32",
		"A2B10G10R10_USCALED_PACK32",
		"A2B10G10R10_SSCALED_PACK32",
		"A2B10G10R10_UINT_PACK32",
		"A2B10G10R10_SINT_PACK32",
		"R16_UNORM",
		"R16_SNORM",
		"R16_USCALED",
		"R16_SSCALED",
		"R16_UINT",
		"R16_SINT",
		"R16_SFLOAT",
		"R16G16_UNORM",
		"R16G16_SNORM",
		"R16G16_USCALED",
		"R16G16_SSCALED",
		"R16G16_UINT",
		"R16G16_SINT",
		"R16G16_SFLOAT",
		"R16G16B16_UNORM",
		"R16G16B16_SNORM",
		"R16G16B16_USCALED",
		"R16G16B16_SSCALED",
		"R16G16B16_UINT",
		"R16G16B16_SINT",
		"R16G16B16_SFLOAT",
		"R16G16B16A16_UNORM",
		"R16G16B16A16_SNORM",
		"R16G16B16A16_USCALED",
		"R16G16B16A16_SSCALED",
		"R16G16B16A16_UINT",
		"R16G16B16A16_SINT",
		"R16G16B16A16_SFLOAT",
		"R32_UINT",
		"R32_SINT",
		"R32_SFLOAT",
		"R32G32_UINT",
		"R32G32_SINT",
		"R32G32_SFLOAT",
		"R32G32B32_UINT",
		"R32G32B32_SINT",
		"R32G32B32_SFLOAT",
		"R32G32B32A32_UINT",
		"R32G32B32A32_SINT",
		"R32G32B32A32_SFLOAT",
		"R64_UINT",
		"R64_SINT",
		"R64_SFLOAT",
		"R64G64_UINT",
		"R64G64_SINT",
		"R64G64_SFLOAT",
		"R64G64B64_UINT",
		"R64G64B64_SINT",
		"R64G64B64_SFLOAT",
		"R64G64B64A64_UINT",
		"R64G64B64A64_SINT",
		"R64G64B64A64_SFLOAT",
		"B10G11R11_UFLOAT_PACK32",
		"E5B9G9R9_UFLOAT_PACK32",
		"D16_UNORM",
		"X8_D24_UNORM_PACK32",
		"D32_SFLOAT",
		"S8_UINT",
		"D16_UNORM_S8_UINT",
		"D24_UNORM_S8_UINT",
		"D32_SFLOAT_S8_UINT",
		"BC1_RGB_UNORM_BLOCK",
		"BC1_RGB_SRGB_BLOCK",
		"BC1_RGBA_UNORM_BLOCK",
		"BC1_RGBA_SRGB_BLOCK",
		"BC2_UNORM_BLOCK",
		"BC2_SRGB_BLOCK",
		"BC3_UNORM_BLOCK",
		"BC3_SRGB_BLOCK",
		"BC4_UNORM_BLOCK",
		"BC4_SNORM_BLOCK",
		"BC5_UNORM_BLOCK",
		"BC5_SNORM_BLOCK",
		"BC6H_UFLOAT_BLOCK",
		"BC6H_SFLOAT_BLOCK",
		"BC7_UNORM_BLOCK",
		"BC7_SRGB_BLOCK",
		"ETC2_R8G8B8_UNORM_BLOCK",
		"ETC2_R8G8B8_SRGB_BLOCK",
		"ETC2_R8G8B8A1_UNORM_BLOCK",
		"ETC2_R8G8B8A1_SRGB_BLOCK",
		"ETC2_R8G8B8A8_UNORM_BLOCK",
		"ETC2_R8G8B8A8_SRGB_BLOCK",
		"EAC_R11_UNORM_BLOCK",
		"EAC_R11_SNORM_BLOCK",
		"EAC_R11G11_UNORM_BLOCK",
		"EAC_R11G11_SNORM_BLOCK",
		"ASTC_4x4_UNORM_BLOCK",
		"ASTC_4x4_SRGB_BLOCK",
		"ASTC_5x4_UNORM_BLOCK",
		"ASTC_5x4_SRGB_BLOCK",
		"ASTC_5x5_UNORM_BLOCK",
		"ASTC_5x5_SRGB_BLOCK",
		"ASTC_6x5_UNORM_BLOCK",
		"ASTC_6x5_SRGB_BLOCK",
		"ASTC_6x6_UNORM_BLOCK",
		"ASTC_6x6_SRGB_BLOCK",
		"ASTC_8x5_UNORM_BLOCK",
		"ASTC_8x5_SRGB_BLOCK",
		"ASTC_8x6_UNORM_BLOCK",
		"ASTC_8x6_SRGB_BLOCK",
		"ASTC_8x8_UNORM_BLOCK",
		"ASTC_8x8_SRGB_BLOCK",
		"ASTC_10x5_UNORM_BLOCK",
		"ASTC_10x5_SRGB_BLOCK",
		"ASTC_10x6_UNORM_BLOCK",
		"ASTC_10x6_SRGB_BLOCK",
		"ASTC_10x8_UNORM_BLOCK",
		"ASTC_10x8_SRGB_BLOCK",
		"ASTC_10x10_UNORM_BLOCK",
		"ASTC_10x10_SRGB_BLOCK",
		"ASTC_12x10_UNORM_BLOCK",
		"ASTC_12x10_SRGB_BLOCK",
		"ASTC_12x12_UNORM_BLOCK",
		"ASTC_12x12_SRGB_BLOCK",
	};

	static size_t vk_format_2_offset = 1000156000;

	static std::vector<const char*> vk_format_2_names = {
		"G8B8G8R8_422_UNORM",
		"B8G8R8G8_422_UNORM",
		"G8_B8_R8_3PLANE_420_UNORM",
		"G8_B8R8_2PLANE_420_UNORM",
		"G8_B8_R8_3PLANE_422_UNORM",
		"G8_B8R8_2PLANE_422_UNORM",
		"G8_B8_R8_3PLANE_444_UNORM",
		"R10X6_UNORM_PACK16",
		"R10X6G10X6_UNORM_2PACK16",
		"R10X6G10X6B10X6A10X6_UNORM_4PACK16",
		"G10X6B10X6G10X6R10X6_422_UNORM_4PACK16",
		"B10X6G10X6R10X6G10X6_422_UNORM_4PACK16",
		"G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16",
		"G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16",
		"G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16",
		"G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16",
		"G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16",
		"R12X4_UNORM_PACK16",
		"R12X4G12X4_UNORM_2PACK16",
		"R12X4G12X4B12X4A12X4_UNORM_4PACK16",
		"G12X4B12X4G12X4R12X4_422_UNORM_4PACK16",
		"B12X4G12X4R12X4G12X4_422_UNORM_4PACK16",
		"G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16",
		"G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16",
		"G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16",
		"G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16",
		"G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16",
		"G16B16G16R16_422_UNORM",
		"B16G16R16G16_422_UNORM",
		"G16_B16_R16_3PLANE_420_UNORM",
		"G16_B16R16_2PLANE_420_UNORM",
		"G16_B16_R16_3PLANE_422_UNORM",
		"G16_B16R16_2PLANE_422_UNORM",
		"G16_B16_R16_3PLANE_444_UNORM",
	};

	// TODO remaining formats

	std::string getVkFormatName(VkFormat f)
	{
		std::string res;
		const size_t i = size_t(f);
		if (i < vk_format_names.size())
		{
			res = vk_format_names[i];
		}
		else if ((i - vk_format_2_offset) < vk_format_2_names.size())
		{
			res = vk_format_2_names[i - vk_format_2_offset];
		}

		if (res.empty())
		{
			res = "Unkown format ("s + std::to_string(f) + ")"s;
		}
		return res;
	}
}