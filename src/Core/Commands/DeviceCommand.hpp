#pragma once

#include "Command.hpp"
#include <Core/Execution/Resource.hpp>
#include <Core/Execution/SynchronizationHelper.hpp>

namespace vkl
{


	class DeviceCommand : public Command
	{

	public:

		template <typename StringLike = std::string>
		constexpr DeviceCommand(VkApplication * app, StringLike && name):
			Command(app, std::forward<StringLike>(name))
		{}

		virtual ~DeviceCommand() override = default;

		virtual bool updateResources(UpdateContext & ctx) override;
	};
}

