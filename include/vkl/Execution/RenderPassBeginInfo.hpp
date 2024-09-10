#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/Commands/ResourceUsageList.hpp>
#include <that/utils/EnumClassOperators.hpp>

namespace vkl
{
	class RenderPassInstance;
	class FramebufferInstance;
	class CommandBuffer;
	class ExecutionContext;

	struct RenderPassBeginInfo
	{	
		enum class Flags : uint32_t
		{
			None = 0,
			ContentsInline = 0x0001,
			ContentsSecondaryCBs = 0x0002,
			ContentsInlineAndSecondaryCBs = ContentsInline | ContentsSecondaryCBs,

		};

		static VkSubpassContents ExtractSubpassContents(Flags flags);

		static constexpr VkRect2D DefaultRenderArea = VkRect2D{ .offset = makeUniformOffset2D(0), .extent = makeUniformExtent2D(uint32_t(-1)) };
		
		std::shared_ptr<RenderPassInstance> render_pass = nullptr;
		std::shared_ptr<FramebufferInstance> framebuffer = nullptr;
		VkRect2D render_area = DefaultRenderArea;
	//protected:
		// bits 0..30: subpass index
		// bit 31: ended subpass (for dynamic rendering)
		uint32_t _internal = 0;
	//public:
		uint32_t clear_value_count = 0;
		VkClearValue* ptr_clear_values = nullptr;

		RenderPassInstance* getRenderPassInstance() const;

		operator bool()const
		{
			return !!getRenderPassInstance();
		}

		void exportResources(ResourceUsageList & resources, bool export_for_all_subpasses = false);

		void recordBegin(ExecutionContext & ctx, Flags flags = Flags::None);

		void exportNextSubpassResources(ResourceUsageList& resources)
		{
			exportSubpassResources(_internal & std::bitMask(31u), resources);
		}

		void exportSubpassResources(uint32_t index, ResourceUsageList& resources);

		void recordEndSubpass(ExecutionContext& ctx);

		void recordNextSubpass(ExecutionContext& ctx, Flags flags = Flags::None);

		void recordEnd(ExecutionContext& ctx);

		void clear()
		{
			render_pass.reset();
			framebuffer.reset();
			render_area = DefaultRenderArea;
			_internal = 0;
			clear_value_count = 0;
			ptr_clear_values = nullptr;
		}

		uint32_t subpassIndex() const
		{
			constexpr const uint32_t subpass_mask = std::bitMask<uint32_t>(31u);
			return _internal & subpass_mask;
		}
	};

	THAT_DECLARE_ENUM_CLASS_OPERATORS(RenderPassBeginInfo::Flags)
}