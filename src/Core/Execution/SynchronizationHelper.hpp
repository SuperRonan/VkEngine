#pragma once

#include <Core/Execution/Resource.hpp>
#include <Core/Execution/ExecutionContext.hpp>

#include <unordered_map>

namespace vkl
{
	// Does not handle very well synchronizing multiple times the same resource
	class SynchronizationHelperV1
	{
	protected:

		std::vector<Resource> _resources;
		std::vector<VkImageMemoryBarrier2> _images_barriers;
		std::vector<VkBufferMemoryBarrier2> _buffers_barriers;

		ExecutionContext& _ctx;

	public:

		SynchronizationHelperV1(ExecutionContext& ctx) :
			_ctx(ctx)
		{}

		void addSynch(Resource const& r);

		void record();
	};

	// Can synchronize correctly the same resource multiple times
	class SynchronizationHelperV2
	{
	protected:

		template <class K, class V>
		using Map = std::unordered_map<K, V>;

		Map<std::shared_ptr<ImageInstance>, std::vector<VkImageMemoryBarrier2>> _images;
		Map<std::shared_ptr<BufferInstance>, std::vector<VkBufferMemoryBarrier2>> _buffers;

		ExecutionContext& _ctx;

	public:

		SynchronizationHelperV2(ExecutionContext& ctx) :
			_ctx(ctx)
		{}

		void addSynch(Resource const& r);

		void record();
	};

	using SynchronizationHelper = SynchronizationHelperV1;
} // namespace vkl
