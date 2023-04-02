#pragma once

#include "Command.hpp"

namespace vkl
{
	class DeviceCommand : public Command
	{
	public:

		class InputSynchronizationHelper
		{
		protected:

			std::vector<VkImageMemoryBarrier2> _images_barriers;
			std::vector<VkBufferMemoryBarrier2> _buffers_barriers;

			ExecutionContext& _ctx;

		public:

			InputSynchronizationHelper(ExecutionContext & ctx):
				_ctx(ctx)
			{}

			void addSynch(Resource const& r);

			void record();

			void declareFinalStates();
		};

	protected:

		

		std::vector<Resource> _resources;

		virtual void clearResources()
		{
			_resources.clear();
		}

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

