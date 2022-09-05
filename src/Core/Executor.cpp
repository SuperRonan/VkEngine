#include "Executor.hpp"

namespace vkl
{
	void Executor::declare(std::shared_ptr<Command> cmd)
	{
		_commands.push_back(cmd);
	}

	void Executor::preprocessCommands()
	{
		for (std::shared_ptr<Command>& cmd : _commands)
		{
			cmd->init();
		}
	}

	void Executor::init()
	{
		preprocessCommands(); 
	}

	void Executor::execute(std::shared_ptr<Command> cmd)
	{
		cmd->execute(_context);
	}
}