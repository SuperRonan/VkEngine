#pragma once

#include <Core/Execution/Resource.hpp>
#include <Core/Execution/ExecutionContext.hpp>

namespace vkl
{
	class SynchronizationHelper
	{
	protected:

		std::vector<Resource> _resources;
		std::vector<VkImageMemoryBarrier2> _images_barriers;
		std::vector<VkBufferMemoryBarrier2> _buffers_barriers;

		ExecutionContext& _ctx;

	public:

		SynchronizationHelper(ExecutionContext& ctx) :
			_ctx(ctx)
		{}

		void addSynch(Resource const& r);

		void record();

		void NotifyContext();
	};
} // namespace vkl
