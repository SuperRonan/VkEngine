#pragma once

#include "Command.hpp"

namespace vkl
{
	class DeviceCommand : public Command
	{
	protected:

		std::vector<Resource> _resources;

	public:

		template <typename StringLike = std::string>
		constexpr DeviceCommand(VkApplication * app, StringLike && name):
			Command(app, std::forward<StringLike>(name))
		{}

		virtual ~DeviceCommand() override = default;

		virtual void recordInputSynchronization(CommandBuffer& cmd, ExecutionContext& context);

		virtual void declareResourcesEndState(ExecutionContext& context);

		virtual bool updateResources() override;
	};
}

