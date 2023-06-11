#pragma once

#include "Command.hpp"

namespace vkl
{
	class InputSynchronizationHelper
	{
	protected:

		std::vector<Resource> _resources;
		std::vector<VkImageMemoryBarrier2> _images_barriers;
		std::vector<VkBufferMemoryBarrier2> _buffers_barriers;

		ExecutionContext& _ctx;

	public:

		InputSynchronizationHelper(ExecutionContext& ctx) :
			_ctx(ctx)
		{}

		void addSynch(Resource const& r);

		void record();

		void NotifyContext();
	};

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

