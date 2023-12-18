#pragma once

#include <utility>
#include <algorithm>
#include <iterator>
#include <initializer_list>
#include <array>
#include <cassert>
#include <memory>

namespace vkl
{
	// TODO Add allocator
	template <class T, size_t MinimumSmallSize = 1>
	class OptVectorImpl1
	{
	public:

		static consteval size_t small_capacity()
		{
			constexpr size_t res = (sizeof(size_t) + sizeof(T*)) / sizeof(T);
			return std::max(res, MinimumSmallSize);
		}

	protected:

		size_t _size = 0;
		using Small = array<T, small_capacity()>;
		struct Large
		{
			size_t capacity = 0;
			T* data = nullptr;
		};
		union
		{
			Small _small;
			Large _large;
		};


		constexpr void growCapacity(size_t c)
		{
			assert(c > capacity());
			if (is_small())
			{
				if (c > small_capacity())
				{
					T * data = new T[c];
					for (size_t i = 0; i < _size; ++i)
					{
						data[i] = std::move(_small[i]);
						_small[i].~T();
					}
					_large.data = data;
					_large.capacity = c;
				}
			}
			else
			{
				Large o = _large;
				
				_large.data = new T[c];
				_large.capacity = c;
				for (size_t i = 0; i < _size; ++i)
				{
					_large.data[i] = std::move(o.data[i]);
				}
				delete[] o.data;
			}
		}

		constexpr void growCapacityIFP(size_t c)
		{
			if (c > capacity())
			{
				growCapacity(c);
			}
		}

		
	public:

		static constexpr void swap(OptVectorImpl1& a, OptVectorImpl1& b)
		{
			std::swap(a._size, b._size);
			if (a.is_small())
			{
				if (b.is_small())
				{
					std::swap(a._small, b._small);
				}
				else
				{
					Large lb = b._large;
					b._small = move(a._small);
					a._large = lb;
				}
			}
			else
			{
				if (b.is_small())
				{
					Large la = a._large;
					a._small = move(b._small);
					b._large = la;
				}
				else
				{
					std::swap(a._large, b._large);
				}
			}
		}

		constexpr ~OptVectorImpl1()
		{
			if (is_small())
			{
				// Have to manyally call the destructor
				for (size_t i = 0; i < _size; ++i)
				{
					_small[i].~T();
				}
			}
			else
			{
				delete[] _large.data;
				_large.data = nullptr;
				_large.capacity = 0;
			}
			_size = 0;
		}

		constexpr bool is_small() const
		{
			return _size < small_capacity();
		}
		

		// (1)
		constexpr OptVectorImpl1() = default;

		// (2)
		// Allocator constructor

		// (3)
		explicit constexpr OptVectorImpl1(size_t count, const T& value) :
			_size(count)
		{
			if (is_small())
			{
				std::fill_n(_small.data(), _size, value);
			}
			else
			{
				_large.capacity = count;
				_large.data = new T[_large.capacity];
				std::fill_n(_large.data, _size, value);
			}
		}

		// (4)
		explicit constexpr OptVectorImpl1(size_t count) :
			_size(count)
		{
			if (is_small())
			{

			}
			else
			{
				_large.capacity = _size;
				_large.data = new T[_large.capacity];
			}
		}

		// (5)
		template <std::input_iterator InputIt>
		constexpr OptVectorImpl1(InputIt first, InputIt const& last) :
			_size(std::distance(first, last))
		{
			if (is_small())
			{
				std::copy_n(first, _size, _small.data());
			}
			else
			{
				_large.capacity = _size;
				_large.data = new T[_large.capacity];
				std::copy_n(first, _size, _large.data);
			}
		}

		// (6)
		constexpr OptVectorImpl1(OptVectorImpl1 const& o) :
			_size(o._size)
		{
			if (is_small())
			{
				std::copy_n(o._small.data(), _size, _small.data());
			}
			else
			{
				_large.capacity = _size;
				_large.data = new T[_large.capacity];
				std::copy_n(o._large.data, _size, _large.data);
			}
		}

		// (7)
		// Copy Constructor with allocator

		// (8)
		constexpr OptVectorImpl1(OptVectorImpl1&& o) noexcept:
			_size(o._size)
		{
			if (is_small())
			{
				_small = move(o._small);
			}
			else
			{
				_large.capacity = o._large.capacity;
				_large.data = o._large.data;
				o._large.capacity = 0;
				o._large.data = nullptr;
				o._size = 0;
			}
		}

		// (9)
		// Move Constructor with allocator

		// (10)		
		constexpr OptVectorImpl1(std::initializer_list<T> init) :
			_size(init.size())
		{
			sizeof(std::initializer_list<T>);
			if (is_small())
			{
				std::copy_n(init.begin(), _size, _small.data());
			}
			else
			{
				_large.capacity = _size;
				_large.data = new T[_large.capacity];
				std::copy_n(init.begin(), _size, _large.data);
			}
		}

		//constexpr OptVectorImpl1(std::initializer_list<T> && init) :
		//	_size(init.size())
		//{
		//	if (is_small())
		//	{
		//		for (size_t i = 0; i < _size; ++i)
		//		{
		//			_small[i] = std::move(init.begin()[i]);
		//		}
		//	}
		//	else
		//	{
		//		_large.capacity = _size;
		//		_large.data = new T[_large.capacity];
		//		for (size_t i = 0; i < _size; ++i)
		//		{
		//			_large.data[i] = std::move(init.begin()[i]);
		//		}
		//	}
		//}

		constexpr bool empty() const
		{
			return _size == 0;
		}

		constexpr operator bool() const
		{
			return !empty();
		}

		constexpr size_t size() const
		{
			return _size;
		}

		constexpr uint32_t size32() const
		{
			return static_cast<uint32_t>(size());
		}

		constexpr size_t capacity()const
		{
			return is_small() ? small_capacity() : _large.capacity;
		}

		T& operator[](size_t i)
		{
			assert(i < size());
			return is_small() ? _small[i] : _large.data[i];
		}

		const T& operator[](size_t i) const
		{
			assert(i < size());
			return is_small() ? _small[i] : _large.data[i];
		}

		T& at(size_t i)
		{
			return operator[](i);
		}

		const T& at(size_t i) const
		{
			return operator[](i);
		}

		T& front()
		{
			assert(size() > 0);
			return is_small() ? _small.front() : _large[0];
		}

		const T& front() const
		{
			assert(size() > 0);
			return is_small() ? _small.front() : _large[0];
		}

		T& back()
		{
			assert(size() > 0);
			return is_small() ? _small.back() : _large[_size - 1];
		}

		const T& back() const
		{
			assert(size() > 0);
			return is_small() ? _small.back() : _large[_size - 1];
		}

		T* data()
		{
			return is_small() ? _small.data() : _large.data;
		}

		const T* data() const
		{
			return is_small() ? _small.data() : _large.data;
		}

		T* begin()
		{
			return data();
		}

		const T* begin() const
		{
			return data();
		}

		const T* cbegin() const
		{
			return data();
		}

		T* end()
		{
			return begin() + size();
		}

		const T* end() const
		{
			return begin() + size();
		}

		const T* cend() const
		{
			return begin() + size();
		}

		constexpr OptVectorImpl1& operator=(const OptVectorImpl1& o)
		{
			growCapacityIFP(o.size());
			_size = o.size();
			std::copy_n(o.data(), _size, data());
			return *this;
		}

		constexpr OptVectorImpl1& operator=(OptVectorImpl1&& o) noexcept
		{
			swap(*this, o);
			return *this;
		}











	};

	template<class T>
	using OptVector = OptVectorImpl1<T>;
}

namespace std
{
	template <class T, size_t M = 1>
	constexpr void swap(vkl::OptVectorImpl1<T, M>& a, vkl::OptVectorImpl1<T, M>& b)
	{
		vkl::OptVectorImpl1<T, M>::swap(a, b);
	}

}