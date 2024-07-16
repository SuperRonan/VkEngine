#pragma once

#include <vkl/Execution/ExecutionContext.hpp>

#include <vkl/Commands/DeviceCommand.hpp>

#include <vkl/VkObjects/RenderPass.hpp>
#include <vkl/VkObjects/DescriptorPool.hpp>
#include <vkl/VkObjects/Swapchain.hpp>
#include <vkl/VkObjects/Queue.hpp>

namespace vkl
{
	class ImguiCommand : public DeviceCommand
	{
	protected:

		std::shared_ptr<Queue> _queue = nullptr;

		std::shared_ptr<Swapchain> _swapchain = nullptr;
		std::vector<std::shared_ptr<Framebuffer>> _framebuffers = {};

		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

		VkFormat _imgui_format = VK_FORMAT_MAX_ENUM;
		VkFormat _imgui_init_format = VK_FORMAT_B8G8R8A8_UNORM;

		Dyn<size_t> _index;

		void createRenderPassIFP();

		void createFramebuffers();

		void initImGui();

		void shutdownImGui();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Swapchain> swapchain = nullptr;
			std::shared_ptr<Queue> queue = nullptr;
		};
		using CI = CreateInfo;

		ImguiCommand(CreateInfo const& ci);

		virtual ~ImguiCommand() override;

		virtual void init()override;

		struct ExecutionInfo
		{
			size_t index;
		};
		using EI = ExecutionInfo;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, ExecutionInfo const& ei);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

		Executable with(ExecutionInfo const& ei);

		Executable operator()(ExecutionInfo const& ei)
		{
			return with(ei);
		}

		virtual bool updateResources(UpdateContext & ctx) override;
	};
}