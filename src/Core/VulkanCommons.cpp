#include "VulkanCommons.hpp"

namespace vkl
{
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
}