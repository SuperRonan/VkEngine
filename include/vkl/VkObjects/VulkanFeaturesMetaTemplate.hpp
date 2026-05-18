#pragma once

#include "VulkanFeaturesMeta.hpp"

namespace vkl::meta
{
	template <> consteval uint FeaturesMeta::GetVkFeaturesStructFlatIndex<VkPhysicalDeviceFeatures>()
	{
		return FeaturesMeta::GetVkFeaturesStructFlatIndex<VkPhysicalDeviceFeatures2>();
	}
} // namespace vkl::meta

#include <vkl/Generated/VulkanFeaturesMeta.hpp>
