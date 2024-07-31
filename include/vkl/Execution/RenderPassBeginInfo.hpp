#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/Commands/ResourceUsageList.hpp>

namespace vkl
{
	class RenderPass;
	class Framebuffer;
	class CommandBuffer;

	struct RenderPassBeginInfo
	{
		enum class Mode
		{
			RENDER_PASS,
			DYNAMIC_RENDERING,
		};
		
		enum class Flags
		{

		};
		
		Mode mode = Mode::RENDER_PASS;
		std::shared_ptr<RenderPass> render_pass = nullptr;
		std::shared_ptr<Framebuffer> framebuffer = nullptr;
		VkRect2D render_area = {};
		MyVector<VkClearValue> clear_values = {};
		VkSubpassContents content = VK_SUBPASS_CONTENTS_INLINE;

		void exportResources(ResourceUsageList & resources);

		void recordBegin(CommandBuffer & cmd);

		void recordNextSubpass(CommandBuffer & cmd, uint32_t index);
	};
}