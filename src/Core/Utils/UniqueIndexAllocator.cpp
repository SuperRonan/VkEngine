#include "UniqueIndexAllocator.hpp"
#include <iostream>

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
		case Policy::AlwaysRecycle:
		{
			bool allocate_on_top = _free_segments.empty() ||
				((_count < _capacity) && (_policy == Policy::AlwaysRecycle));

			if (allocate_on_top)
			{
				res = _count;
				++_count;
			}
			else
			{
				Segment & s = _free_segments.front();
				res = s.begin;
				if (s.len == 1)
				{
					_free_segments.pop_front();
				}
				else
				{
					++s.begin;
					--s.len;
				}
			}
		}
		break;
		}
		if (_count > _capacity)
		{
			_capacity *= 2;
		}
		return res;
	}

	UniqueIndexAllocator::Index UniqueIndexAllocator::allocate(Index count)
	{
		NOT_YET_IMPLEMENTED;
		return 0;
	}

	// TODO
	// _free_segments is sorted -> can do a dichotomic search rather than a linear search

	void UniqueIndexAllocator::release(Index index)
	{
		if (_policy == Policy::FastButWasteful)
		{
			if ((index + 1) == _count)
			{
				--_count;
			}
		}
		else
		{
			if ((index + 1) == _count)
			{
				--_count;
				if (!_free_segments.empty())
				{
					Segment & l = _free_segments.back();
					if (l.end() == index) 
					{
						_count = l.begin;
						_free_segments.pop_back();
					}
				}
			}
			else
			{
				auto it = _free_segments.begin();
				bool broke = false;
				while (it != _free_segments.end())
				{
					if ((it->begin - 1) == index) // Append left of segment
					{
						--it->begin;
						++it->len;
						broke = true;
						break;
					}
					else
					{
						if (it->end() == index) // Append right of segment
						{
							if (auto next = std::next(it); (next != _free_segments.end()) && (next->begin == (it->end() + 1))) // Can merge it and next
							{
								Index prev_end = next->end();
								next->begin = it->begin;
								next->len += (it->len + 1);
								assert(prev_end == next->end());
								auto _it = _free_segments.erase(it);
							}
							else
							{
								++it->len;
							}
							broke = true;
							break;
						}
						else if (it->begin > index) // Insert a new segment
						{
							Segment s{.begin = index, .len = 1};
							_free_segments.insert(it, s);
							broke = true;
							break;
						}
						else
						{
							++it;
						}
					}
				}
				if (!broke)
				{
					Segment s{ .begin = index, .len = 1 };
					_free_segments.insert(it, s);
				}
			}
		}

		assert(checkIntegrity());
	}

	void UniqueIndexAllocator::release(Index index, Index count)
	{
		NOT_YET_IMPLEMENTED;
	}

	bool UniqueIndexAllocator::isAllocated(Index index) const
	{
		bool res = true;
		if (index >= _count)
		{
			res = false;
		}
		else
		{
			auto it = _free_segments.begin();
			while (it != _free_segments.end())
			{
				if (it->contains(index))
				{
					res = false;
					break;
				}
				else if (it->begin > index)
				{
					res = true;
					break;
				}
				++it;
			}
		}
		return res;
	}

	bool UniqueIndexAllocator::checkIntegrity() const
	{
		bool res = true;
		res &= (_count <= _capacity);

		auto it = _free_segments.begin();
		while (it != _free_segments.end())
		{
			res &= (it->len > 0);
			if (auto next = std::next(it); next != _free_segments.end())
			{
				res &= (next->begin > it->end());
			}
			++it;
		}

		return res;
	}

	void UniqueIndexAllocator::print(std::ostream& out)
	{
		auto it = _free_segments.begin();
		while (it != _free_segments.end())
		{
			out << "[" << it->begin << ".." << (it->end() - 1) << "], ";
			++it;
		}
		out << "[" << _count << "... (" << _capacity << ")\n";
	}
}