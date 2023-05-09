#pragma once

#include <Core/ExecutionContext.hpp>
#include <Core/DeviceCommand.hpp>
#include <Core/RenderPass.hpp>
#include <Core/DescriptorPool.hpp>
#include <Core/Swapchain.hpp>

namespace vkl
{
	class ImguiCommand : public DeviceCommand
	{
	protected:

		std::shared_ptr<Swapchain> _swapchain = nullptr;
		std::vector<std::shared_ptr<Framebuffer>> _framebuffers = {};

		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

		size_t _index = 0;

		void createRenderPass();

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

		virtual bool updateResources() override;

		constexpr void setIndex(size_t index)
		{
			_index = index;
		}
	};
}