#include "UniqueIndexAllocator.hpp"

namespace vkl
{
	UniqueIndexAllocator::Index UniqueIndexAllocator::allocate()
	{
		Index res = 0;
		switch (_policy)
		{
		case Policy::FastButWasteful:
		{
			res = _count;
			++_count;
		}
		break;
		case Policy::FitCapacity:
		{
			NOT_YET_IMPLEMENTED;
		}
		break;
		case Policy::AlwaysRecycle:
		{
			NOT_YET_IMPLEMENTED;
		}
		break;
		}
		if (_count > _capacity)
		{
			_capacity *= 2;
		}
		return res;
	}

	void UniqueIndexAllocator::release(Index index)
	{
		switch (_policy)
		{
		case Policy::FastButWasteful:
		{
			// If freeing the latest index
			if ((index + 1) == _count)
			{
				--_count;
			}
		}
		break;
		case Policy::FitCapacity:
		{
			NOT_YET_IMPLEMENTED;
		}
		break;
		case Policy::AlwaysRecycle:
		{
			NOT_YET_IMPLEMENTED;
		}
		break;
		}
	}
}