#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/Maths/AlignedAxisBoundingBox.hpp>

namespace vkl
{
	class UniqueIndexAllocator
	{
	public:

		using Index = uint32_t;

	protected:

		using Segment = Range<Index>;

		bool _recycle = false;

		Index _num_allocated = 0;
		Index _capacity = 1;

		// Sorted, keep track of free segments when not using FastButWasteful
		std::deque<Segment> _free_segments = {};

	public:

		UniqueIndexAllocator(Index initial_capacity=1, bool recycle=true);

		// Clear allocated, does not alter capacity
		void clear();

		Index allocate();

		Index allocate(Index count);

		void release(Index index);

		void release(Index index, Index count);

		bool isAllocated(Index index) const;

		Index capacity()const
		{
			return _capacity;
		}

		void growCapacity();

		// Release excess capacity
		void shrinkToFit();

		bool checkIntegrity() const;
		
		void print(std::ostream & stream);
	};
}