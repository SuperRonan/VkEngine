#pragma once

#include <Core/VulkanCommons.hpp>
#include <Core/Maths/AlignedAxisBoundingBox.hpp>

namespace vkl
{
	class UniqueIndexAllocator
	{
	public:

		enum class Policy
		{
			FastButWasteful,
			FitCapacity,
			AlwaysRecycle,
		};
		
		using Index = uint32_t;

	protected:

		using Segment = AlignedAxisBoundingBox<1, Index>;

		Policy _policy = Policy::FastButWasteful;

		std::deque<Segment> _free_segments;

		Index _count = 0;
		Index _capacity = 1;

	public:

		UniqueIndexAllocator(Policy policy = Policy::FastButWasteful):
			_policy(policy)
		{}

		Index allocate();

		void release(Index index);

		Index count()const
		{
			return _count;
		}

		Index capacity()const
		{
			return _capacity;
		}
	};
}