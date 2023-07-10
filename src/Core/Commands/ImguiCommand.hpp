#pragma once

#include <Core/Execution/ExecutionContext.hpp>
#include <Core/Commands/DeviceCommand.hpp>
#include <Core/VkObjects/RenderPass.hpp>
#include <Core/VkObjects/DescriptorPool.hpp>
#include <Core/VkObjects/Swapchain.hpp>

namespace vkl
{
	class ImguiCommand : public DeviceCommand
	{
	protected:

		std::shared_ptr<Swapchain> _swapchain = nullptr;
		std::vector<std::shared_ptr<Framebuffer>> _framebuffers = {};

		VkFormat _render_pass_format;
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

		size_t _index = 0;

		void createRenderPass();

		void maybeDestroyRenderPass();

		void createFramebuffers();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Swapchain> swapchain = nullptr;
		};
		using CI = CreateInfo;

		ImguiCommand(CreateInfo const& ci);

		virtual void init()override;

		struct ExecutionInfo
		{
			size_t index;
		};
		using EI = ExecutionInfo;

		void execute(ExecutionContext& ctx, ExecutionInfo const& ei);

		virtual void execute(ExecutionContext& context) override;

		Executable with(ExecutionInfo const& ei);

		Executable operator()(ExecutionInfo const& ei)
		{
			return with(ei);
		}
		
		virtual ~ImguiCommand() override;

		virtual bool updateResources(UpdateContext & ctx) override;

		constexpr void setIndex(size_t index)
		{
			_index = index;
		}
	};
}