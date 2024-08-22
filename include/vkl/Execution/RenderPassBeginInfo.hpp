#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/Commands/ResourceUsageList.hpp>

namespace vkl
{
	class RenderPassInstance;
	class FramebufferInstance;
	class CommandBuffer;
	class ExecutionContext;

	struct RenderPassBeginInfo
	{	
		enum class Flags
		{
			None = 0,
		};

		static constexpr VkRect2D DefaultRenderArea = VkRect2D{ .offset = makeUniformOffset2D(0), .extent = makeUniformExtent2D(uint32_t(-1)) };
		
		std::shared_ptr<RenderPassInstance> render_pass = nullptr;
		std::shared_ptr<FramebufferInstance> framebuffer = nullptr;
		VkRect2D render_area = DefaultRenderArea;
	protected:
		uint32_t _subpass_index = 0;
	public:
		uint32_t clear_value_count = 0;
		VkClearValue* ptr_clear_values = nullptr;
		MyVector<VkClearValue> clear_values = {};

		RenderPassInstance* getRenderPassInstance() const;

		operator bool()const
		{
			return !!getRenderPassInstance();
		}

		void exportResources(ResourceUsageList & resources, bool export_for_all_subpasses = false);

		void recordBegin(ExecutionContext & ctx, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

		void exportNextSubpassResources(ResourceUsageList& resources)
		{
			exportSubpassResources(_subpass_index + 1, resources);
		}

		void exportSubpassResources(uint32_t index, ResourceUsageList& resources);

		void recordNextSubpass(ExecutionContext& ctx, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

		void recordEnd(ExecutionContext& ctx);

		void clear()
		{
			render_pass.reset();
			framebuffer.reset();
			render_area = DefaultRenderArea;
			_subpass_index = 0;
			clear_value_count = 0;
			ptr_clear_values = nullptr;
			clear_values.clear();
		}
	};
}