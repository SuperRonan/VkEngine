#pragma once

#include "ExecutionContext.hpp"
#include "Command.hpp"
#include "VkWindow.hpp"
#include <queue>

namespace vkl
{
	class Executor : public VkObject
	{
	protected:

		std::shared_ptr<CommandBuffer> _command_buffer_to_submit;
		
		ResourceStateTracker _resources_state;

		ExecutionContext _context;
		
		std::vector<std::shared_ptr<Command>> _commands;

		void preprocessCommands();

		struct InBetween
		{
			std::shared_ptr<Fence> fence = nullptr;
			std::shared_ptr<Semaphore> semaphore = nullptr;
			std::shared_ptr<CommandBuffer> prev_cb = nullptr;
		};

		void prepareSubmission();

		std::queue<InBetween> _previous_in_betweens;
		InBetween _in_between;

	public:

		template <typename StringLike = std::string>
		Executor(VkApplication* app = nullptr, StringLike&& name = {}):
			VkObject(app, std::forward<StringLike>(name)),
			_context(&_resources_state, nullptr)
		{}

		void declare(std::shared_ptr<Command> cmd);

		void init();

		void beginCommandBuffer();

		void endCommandBufferAndSubmit();

		void present(VkWindow * window, uint32_t present_index);

		void execute(std::shared_ptr<Command> cmd);

		void submit();

		void waitForCurrentCompletion(uint64_t timeout = UINT64_MAX);

	};
}