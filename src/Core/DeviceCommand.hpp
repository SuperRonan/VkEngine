#pragma once

#include "Command.hpp"

namespace vkl
{
	class DeviceCommand : public Command
	{
	protected:

		std::vector<Resource> _resources;

	public:

		virtual void recordInputSynchronization(CommandBuffer& cmd, ExecutionContext& context);

	};
}

