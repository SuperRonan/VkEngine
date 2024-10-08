#pragma once

#include <vkl/Execution/ExecutionContext.hpp>

#include <vkl/Commands/DeviceCommand.hpp>

#include <vkl/VkObjects/RenderPass.hpp>
#include <vkl/VkObjects/DescriptorPool.hpp>
#include <vkl/VkObjects/Queue.hpp>
#include <vkl/VkObjects/Fence.hpp>
#include <vkl/VkObjects/VkWindow.hpp>

namespace vkl
{
	class ImguiCommand : public DeviceCommand
	{
	protected:

		std::shared_ptr<Queue> _queue = nullptr;

		std::shared_ptr<VkWindow> _target_window = nullptr;
		std::vector<std::shared_ptr<Framebuffer>> _framebuffers = {};

		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

		VkFormat _imgui_init_format = VK_FORMAT_B8G8R8A8_UNORM;

		Dyn<size_t> _index;

		MyVector<std::shared_ptr<Fence>> _fences_to_wait = {};

		ColorCorrectionInfo _color_correction_info = {};

		bool _re_create_imgui_pipeline = true;

		void createRenderPassIFP();

		void createFramebuffers();

		void initImGui();

		void shutdownImGui();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<VkWindow> target_window = nullptr;
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

		void setFenceToWait(uint index, std::shared_ptr<Fence> const& fence)
		{
			_fences_to_wait[index] = fence;
		}

		void clearFencesToWait()
		{
			for (uint32_t i = 0; i < _fences_to_wait.size32(); ++i)
			{
				_fences_to_wait[i].reset();
			}
		}
	};
}