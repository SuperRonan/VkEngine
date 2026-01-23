#include <vkl/Utils/UniqueIndexAllocator.hpp>
#include <iostream>
#include <algorithm>

namespace vkl
{
	UniqueIndexAllocator::UniqueIndexAllocator(Index initial_capacity, bool recycle) :
		_recycle(recycle),
		_capacity(initial_capacity)
	{
		clear();
	}
	
	void UniqueIndexAllocator::clear()
	{
		_num_allocated = 0;
		if (_recycle)
		{
			_free_segments = {
				Segment{.begin = 0, .len = _capacity},
			};
		}
	}

	void UniqueIndexAllocator::growCapacity()
	{
		Index old_capacity = _capacity;
		_capacity *= 2;
		if (_recycle)
		{
			if (_free_segments.empty() || _free_segments.back().end() != old_capacity)
			{
				_free_segments.push_back(Segment{.begin = old_capacity, .len = _capacity - old_capacity});
			}
			else
			{
				_free_segments.back().len += (_capacity - old_capacity);
			}
		}
	}

	void UniqueIndexAllocator::shrinkToFit()
	{
		if (_recycle && !_free_segments.empty())
		{
			auto& back = _free_segments.back();
			if (back.end() == _capacity)
			{
				_capacity = back.begin;
				_free_segments.pop_back();
			}
		}
	}

	UniqueIndexAllocator::Index UniqueIndexAllocator::allocate()
	{
		++_num_allocated;
		if (_num_allocated > _capacity)
		{
			growCapacity();
		}
		Index res = 0;
		if (!_recycle)
		{
			res = _num_allocated;
		}
		else
		{
			Segment& s = _free_segments.front();
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
		assert(checkIntegrity());
		return res;
	}

	UniqueIndexAllocator::Index UniqueIndexAllocator::allocate(Index count)
	{
		VKL_NOT_YET_IMPLEMENTED;
		return 0;
	}

	void UniqueIndexAllocator::release(Index index)
	{
		if (!_recycle)
		{
			if ((index + 1) == _num_allocated)
			{
				--_num_allocated;
			}
		}
		else
		{
			--_num_allocated;
			assert(!isAllocated(index));
			if (_free_segments.empty())
			{
				_free_segments.push_back(Segment{ .begin = index, .len = 1 });
			}
			else if (Segment& back = _free_segments.back(); index >= back.end())
			{
				if (index == back.end())
				{
					++back.len;
				}
				else
				{
					_free_segments.push_back(Segment{ .begin = index, .len = 1 });
				}
			}
			else if (Segment& front = _free_segments.front(); index < front.begin)
			{
				if (index + 1 == front.begin)
				{
					++front.len;
					--front.begin;
				}
				else
				{
					_free_segments.push_front(Segment{ .begin = index, .len = 1 });
				}
			}
			else
			{
				assert(_free_segments.size() >= 2);
				auto right = std::upper_bound(_free_segments.begin(), _free_segments.end(), index, [](Index index, Segment segment)
				{
					return index < segment.end();
				});
				assert(right != _free_segments.end());
				assert(right != _free_segments.begin());
				auto  left = std::prev(right);

				if (left->end() + 1 == right->begin) // Merge left and right
				{
					assert(left->end() == index);
					assert(index + 1 == right->begin);
					left->len += right->len + 1;
					_free_segments.erase(right);
				}
				else if (left->end() == index) // Expand left by one unit to the right
				{
					++left->len;
				}
				else if (index + 1 == right->begin) // Expand right by one unit to the left
				{
					++right->len;
					--right->begin;
				}
				else // Insert an intermediate segment between left and right
				{
					_free_segments.insert(right, Segment{ .begin = index, .len = 1 });
				}
			}
		}
		assert(checkIntegrity());
	}

	void UniqueIndexAllocator::release(Index index, Index count)
	{
		// Not the most efficient
		for (Index i = 0; i < count; ++i)
		{
			release(index + i);
		}
	}

	bool UniqueIndexAllocator::isAllocated(Index index) const
	{
		bool res = true;
		if (index >= _num_allocated)
		{
			res = false;
		}
		else
		{
			auto it = std::upper_bound(_free_segments.begin(), _free_segments.end(), index, [](Index lhs, Segment rhs)
			{
				return lhs >= rhs.begin;
			});
			if (it == _free_segments.end())
			{
				res = false;
			}
			else
			{
				res = index < it->end();
			}
		}
		return res;
	}

	bool UniqueIndexAllocator::checkIntegrity() const
	{
		bool res = true;
		if (!(_num_allocated <= _capacity))
		{
			res = false;
		}

		Index num_free = 0;
		auto it = _free_segments.begin();
		while (it != _free_segments.end())
		{
			num_free += it->len;
			if (!(it->len > 0))
			{
				res = false;
			}
			auto next = std::next(it);
			if (next != _free_segments.end())
			{
				if (!(next->begin > it->end()))
				{
					res = false;
				}
			}
			it = std::move(next);
		}
		if (_recycle)
		{
			if (!((num_free + _num_allocated) == _capacity))
			{
				res = false;
			}
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
		out << "[" << _num_allocated << "... (" << _capacity << ")\n";
	}
}