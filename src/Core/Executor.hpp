#pragma once

#include "ExecutionContext.hpp"
#include "Command.hpp"

namespace vkl
{
	class Executor : public VkObject
	{
	protected:

		std::vector<std::shared_ptr<CommandBuffer>> _command_buffers_to_submit;
		ExecutionContext _context;
		
		std::vector<std::shared_ptr<Command>> _commands;

		void preprocessCommands();

	public:

		void declare(std::shared_ptr<Command> cmd);

		void init();


		void execute(std::shared_ptr<Command> cmd);

		void submit();

		void wait();

	};
}