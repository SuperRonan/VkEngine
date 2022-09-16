#pragma once

#include "ExecutionContext.hpp"
#include "Command.hpp"
#include "VkWindow.hpp"
#include <queue>
#include "TransferCommand.hpp"

namespace vkl
{
	class LinearExecutor : public VkObject
	{
	protected:

		std::shared_ptr<VkWindow> _window = nullptr;

		VkWindow::AquireResult _aquired;

		std::shared_ptr<CommandBuffer> _command_buffer_to_submit = nullptr;
		
		ResourceStateTracker _resources_state = {};

		ExecutionContext _context;

		std::shared_ptr<BlitImage> blit_to_final;
		std::vector<std::shared_ptr<Command>> _commands = {};

		void preprocessCommands();

		struct InBetween
		{
			std::shared_ptr<Fence> fence = nullptr;
			std::shared_ptr<Semaphore> semaphore = nullptr;
			std::shared_ptr<CommandBuffer> prev_cb = nullptr;
		};

		void stackInBetween();

		std::queue<InBetween> _previous_in_betweens;
		InBetween _in_between;

	public:

		template <typename StringLike = std::string>
		LinearExecutor(std::shared_ptr<VkWindow> window , StringLike&& name = {}):
			VkObject(window->application(), std::forward<StringLike>(name)),
			_window(window),
			_context(&_resources_state, nullptr)
		{}

		void declare(std::shared_ptr<Command> cmd);

		void init();

		void beginFrame();

		void beginCommandBuffer();

		void preparePresentation(std::shared_ptr<ImageView> img_to_present);

		void endCommandBufferAndSubmit();

		void present();

		void execute(std::shared_ptr<Command> cmd);

		void submit();

		void waitForCurrentCompletion(uint64_t timeout = UINT64_MAX);

	};
}