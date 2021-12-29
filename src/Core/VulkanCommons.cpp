#include "VulkanCommons.hpp"

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mips)
{
	VkImageViewCreateInfo view_ci{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange = VkImageSubresourceRange{
			.aspectMask = aspect,
			.baseMipLevel = 0,
			.levelCount = mips,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};
	VkImageView res;
	VK_CHECK(vkCreateImageView(device, &view_ci, nullptr, &res), "Failed to create an image view.");
	return res;
}