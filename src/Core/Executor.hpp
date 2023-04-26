#pragma once

#include "ExecutionContext.hpp"
#include "Command.hpp"
#include "VkWindow.hpp"
#include <queue>
#include "TransferCommand.hpp"
#include "ImguiCommand.hpp"

namespace vkl
{
	class LinearExecutor : public VkObject
	{
	protected:

		size_t _frame_index = size_t(-1);

		std::shared_ptr<VkWindow> _window = nullptr;

		VkWindow::AquireResult _aquired;

		std::shared_ptr<CommandBuffer> _command_buffer_to_submit = nullptr;
		
		ResourceStateTracker _resources_state = {};

		ExecutionContext _context;

		std::shared_ptr<BlitImage> _blit_to_present = nullptr;
		std::shared_ptr<ImguiCommand> _render_gui = nullptr;

		std::vector<std::shared_ptr<Command>> _commands = {};

		std::vector<std::shared_ptr<ImageView>> _registered_images = {};
		std::vector<std::shared_ptr<Buffer>> _registered_buffers = {};


		void preprocessCommands();

		void preprocessResources();

		struct InBetween
		{
			std::shared_ptr<CommandBuffer> prev_cb = nullptr;
			std::shared_ptr<CommandBuffer> next_cb = nullptr;
			std::vector<std::shared_ptr<Fence>> fences = {};
			std::shared_ptr<Semaphore> semaphore = nullptr;
			std::vector<std::shared_ptr<VkObject>> dependecies = {};
		};

		void stackInBetween();

		std::queue<InBetween> _previous_in_betweens;
		InBetween _in_between;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<VkWindow> window = nullptr;
			bool use_ImGui = false;
		};

		using CI = CreateInfo;

		template <typename StringLike = std::string>
		LinearExecutor(std::shared_ptr<VkWindow> window , StringLike&& name = {}):
			VkObject(window->application(), std::forward<StringLike>(name)),
			_window(window),
			_context(&_resources_state, nullptr)
		{}

		LinearExecutor(CreateInfo const& ci);

		virtual ~LinearExecutor();

		void declare(std::shared_ptr<Command> cmd);

		void declare(std::shared_ptr<ImageView> view);

		void declare(std::shared_ptr<Buffer> buffer);

		void init();

		void updateResources();

		void beginFrame();

		void beginCommandBuffer();

		void preparePresentation(std::shared_ptr<ImageView> img_to_present, bool render_ImGui = true);

		void endCommandBufferAndSubmit();

		void present();

		void execute(std::shared_ptr<Command> cmd);

		void execute(Executable const& executable);

		void operator()(std::shared_ptr<Command> cmd)
		{
			execute(cmd);
		}

		void operator()(Executable const& executable)
		{
			execute(executable);
		}

		void submit();

		void waitForAllCompletion(uint64_t timeout = UINT64_MAX);

		void waitForCurrentCompletion(uint64_t timeout = UINT64_MAX);

		std::shared_ptr<CommandBuffer> getCommandBuffer();
	};
}