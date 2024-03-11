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

		using Segment = Range<Index>;

		Policy _policy = Policy::FastButWasteful;

		// Sorted
		std::deque<Segment> _free_segments;

		Index _count = 0;
		Index _capacity = 1;

	public:

		UniqueIndexAllocator(Policy policy = Policy::FastButWasteful):
			_policy(policy)
		{}

		Index allocate();

		Index allocate(Index count);

		void release(Index index);

		void release(Index index, Index count);

		bool isAllocated(Index index) const;

		Index count()const
		{
			return _count;
		}

		Index capacity()const
		{
			return _capacity;
		}

		bool checkIntegrity() const;
		
		void print(std::ostream & stream);
	};
}