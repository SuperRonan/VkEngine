#pragma once

#include <utility>
#include <algorithm>
#include <iterator>
#include <initializer_list>
#include <array>
#include <cassert>
#include <memory>
#include <vector>

#include <Core/Utils/Container.hpp>

namespace vkl
{
	template <class T, size_t MinimumSmallSize = 1>
	class SmallVectorBase
	{
	public:

		static consteval size_t small_capacity()
		{
			constexpr size_t res = (sizeof(size_t) + sizeof(T*)) / sizeof(T);
			return std::max(res, MinimumSmallSize);
		}
		
	};

	// TODO a OptVector where:
	// Store data in _small if possible and _large is not init yet

	enum class SmallVectorPolicy
	{
		TargetSmall,
		TargetLarge,
	};

	template <class T, size_t MinimumSmallSize = 1, SmallVectorPolicy policy = SmallVectorPolicy::TargetSmall, class Allocator = std::allocator<T>>
	class OptVectorImpl1 : public SmallVectorBase<T, MinimumSmallSize>
	{
		static_assert(MinimumSmallSize < 255);
	protected:

		using BaseType = typename SmallVectorBase<T, MinimumSmallSize>;

		using std_vector_type = typename std::vector<T, Allocator>;
		using value_type = typename std_vector_type::value_type;
		using allocator_type = typename std_vector_type::allocator_type;
		using size_type = typename std_vector_type::size_type;
		using difference_type = typename std_vector_type::difference_type;
		using reference = typename std_vector_type::reference;
		using const_reference = typename std_vector_type::const_reference;
		using pointer = typename std_vector_type::pointer;
		using const_pointer = typename std_vector_type::const_pointer;
		using iterator = pointer;
		using const_iterator = const_pointer;

		using small_size_type = uint8_t;

		small_size_type _small_size = 0;
		
		std::array<T, BaseType::small_capacity()> _small;
		std_vector_type _large;

		void saturateSmallSize()
		{
			_small_size = small_size_type(~0);
		}

		void clearSmall()
		{
			for (size_t i = 0; i < BaseType::small_capacity(); ++i)
			{
				if (i < _small_size)
				{
					_small[i].~T();
				}
			}
		}

		void moveSmallToLarge()
		{
			assert(_large.size() >= _small_size);
			//std::move(_small.begin(), _small.begin() + _small_size, _large.begin());
			for (size_t i = 0; i < BaseType::small_capacity(); ++i)
			{
				if (i < _small_size)
				{
					_large[i] = std::move(_small[i]);
				}
			}
			saturateSmallSize();
		}

		void moveLargeToSmall()
		{
			assert(_large.size() <= BaseType::small_capacity());
			_small_size = static_cast<small_size_type>(_large.size());
			for (size_t i = 0; i < BaseType::small_capacity(); ++i)
			{
				if (i < _small_size)
				{
					_small[i] = std::move(_large[i]);
				}
			}
			_large.clear();
		}

	public:

		constexpr bool is_small()const
		{
			return _small_size <= BaseType::small_capacity();
		}

		static constexpr void swap(OptVectorImpl1& a, OptVectorImpl1& b)
		{
			std::swap(a._small_size, b._small_size);
			std::swap(a._small, b._small);
			std::swap(a._large, b._large);
			//if (a.is_small())
			//{
			//	if (b.is_small())
			//	{
			//		std::swap(a._small, b._small);
			//	}
			//	else
			//	{
			//		std::vector tmp = std::move(b._large);
			//		b._small = std::move(a._small);
			//		a._large = std::move(tmp);
			//	}
			//}
			//else
			//{
			//	if (b.is_small())
			//	{
			//		std::vector tmp = std::move(a._large);
			//		a._small = std::move(b._small);
			//		b._large = std::move(tmp);
			//	}
			//	else
			//	{
			//		std::swap(a._large, b._large);
			//	}
			//}
			//std::swap(a._small_size, b._small_size);	
		}

		constexpr ~OptVectorImpl1()
		{
			if (is_small())
			{
				clearSmall();
			}
			else
			{
				
			}
			_small_size = 0;
		}

		// (1)
		constexpr OptVectorImpl1() noexcept(noexcept(Allocator())) = default;

		// (2)
		constexpr explicit OptVectorImpl1(const Allocator & alloc) noexcept :
			_small_size(0),
			_small(),
			_large(alloc)
		{}

		// (3)
		constexpr OptVectorImpl1(size_type count, const T& value = T(), const Allocator& alloc = Allocator())
		{
			if (count <= BaseType::small_capacity())
			{
				_small_size = count;
				for (size_type i = 0; i < count; ++i)
				{
					_small[i] = value;
				}
			}
			else
			{
				saturateSmallSize();
				_large = std_vector_type(count, value, alloc);
			}
		}

		// (4)
		constexpr explicit OptVectorImpl1(size_type count)
		{
			if (count <= BaseType::small_capacity())
			{
				_small_size = count;
			}
			else
			{
				saturateSmallSize();
				_large = std_vector_type(count);
			}
		}

		// (5) TODO add allocator
		template <std::input_iterator InputIt>
		constexpr OptVectorImpl1(InputIt first, InputIt last)
		{
			const size_t len = std::distance(first, last);
			if (len <= BaseType::small_capacity())
			{
				_small_size = len;
				std::copy_n(first, len, _small.begin());
			}
			else
			{
				_large = std_vector_type(first, last);
			}
		}

		// (6)
		constexpr OptVectorImpl1(const OptVectorImpl1& other) :
			_small_size(other._small_size)
		{
			if (is_small())
			{
				std::copy_n(other._small.begin(), _small_size, _small.begin());
			}
			else
			{
				_large = other._large;
			}
		}

		// (7) Copy with allocator

		// (8)
		constexpr OptVectorImpl1(OptVectorImpl1&& other) :
			_small_size(other._small_size)
		{
			if (is_small())
			{
				for (size_t i = 0; i < BaseType::small_capacity(); ++i)
				{
					if (i < _small_size)
					{
						_small[i] = std::move(other._small[i]);
					}
				}
			}
			else
			{
				_large = std::move(other._large);
			}
		}

		// (9) move constructor with allocator

		// (10)
		constexpr OptVectorImpl1(std::initializer_list<T> const& init)
		{
			if (init.size() <= BaseType::small_capacity())
			{
				_small_size = init.size();
				std::copy_n(init.begin(), _small_size, _small.begin());
			}
			else
			{
				saturateSmallSize();
				_large = init;
			}
		}

		// (11) range C++23


		// (1)
		constexpr void assign(const OptVectorImpl1& other)
		{
			if (other.is_small())
			{
				if constexpr (policy == SmallVectorPolicy::TargetSmall)
				{
					_large.clear();
					_small = other._small;
					_small_size = other._small_size;
				}
				else
				{
					if (is_small())
					{
						_small = other._small;
						_small_size = other._small_size;
					}
					else
					{
						_large.resize(other._small_size);
						for (size_type i = 0; i < other.small_capacity(); ++i)
						{
							if (i < other._small_size)
							{
								_large[i] = other._small[i];
							}
						}
					}
				}
			}
			else
			{
				if (is_small())
				{
					clearSmall();
				}
				_large = other._large;
				_small_size = other._small_size;
			}
		}

		// (2)
		constexpr void assign(OptVectorImpl1&& other)
		{
			if (other.is_small())
			{
				if constexpr (policy == SmallVectorPolicy::TargetSmall)
				{
					_large.clear();
					_small = std::move(other._small);
					_small_size = other._small_size;
				}
				else
				{
					if (is_small())
					{
						_small = std::move(other._small);
					}
					else
					{
						_large.resize(other._small_size);
						for (size_type i = 0; i < other.small_capacity(); ++i)
						{
							if (i < other._small_size)
							{
								_large[i] = std::move(other._small[i]);
							}
						}
					}
				}
			}
			else
			{
				if (is_small())
				{
					clearSmall();
				}
				_large = std::move(other._large);
				_small_size = other._small_size;
			}	
		}

		// (3)
		constexpr void assign(std::initializer_list<T> ilist)
		{
			if constexpr (policy == SmallVectorPolicy::TargetSmall)
			{
				if (ilist.size() <= BaseType::small_capacity()) // Can be small
				{
					_large.clear();
					std::copy_n(ilist.begin(), ilist.size(), _small.begin());
					_small_size = ilist.size();
				}
				else
				{
					if (is_small())
					{
						clearSmall();
					}
					saturateSmallSize();
					_large = ilist;
				}
			}
			else
			{
				if (is_small())
				{
					if (ilist.size() <= BaseType::small_capacity()) // Can be small
					{
						std::copy_n(ilist.begin(), ilist.size(), _small.begin());
						_small_size = ilist.size();
					}
					else
					{
						clearSmall();
						saturateSmallSize();
						_large = ilist;
					}
				}
				else
				{
					_large = ilist;
				}
			}
		}

		// (1)
		constexpr OptVectorImpl1& operator=(const OptVectorImpl1& other)
		{
			assign(other);
			return *this;
		}

		// (2)
		constexpr OptVectorImpl1& operator=(OptVectorImpl1&& other)
		{
			assign(std::move(other));
			return *this;
		}

		// (3)
		constexpr OptVectorImpl1& operator=(std::initializer_list<T> ilist)
		{
			assign(ilist);
			return *this;
		}

		constexpr size_type size()const
		{
			return is_small() ? _small_size : _large.size();
		}

		constexpr uint32_t size32()const
		{
			return static_cast<uint32_t>(size());
		}

		constexpr bool empty()const
		{
			bool res;
			if constexpr (policy == SmallVectorPolicy::TargetSmall)
			{
				res = _small_size == 0;
			}
			else
			{
				res = (_small_size == 0) || _large.empty();
			}
			return res;
		}

		constexpr operator bool()const
		{
			return !empty();
		}

		constexpr T* data()
		{
			return is_small() ? _small.data() : _large.data();
		}

		constexpr const T* data() const
		{
			return is_small() ? _small.data() : _large.data();
		}

		constexpr T& operator[](size_type i)
		{
			return data()[i];
		}

		constexpr const T& operator[](size_type i) const
		{
			return data()[i];
		}

		constexpr T& at(size_type i)
		{
			return operator[](i);
		}

		constexpr const T& at(size_type i) const
		{
			return operator[](i);
		}

		constexpr T& front()
		{
			assert(!empty());
			return is_small() ? _small.front() : _large.front();
		}

		constexpr const T& front()const
		{
			assert(!empty());
			return is_small() ? _small.front() : _large.front();
		}

		constexpr T& back()
		{
			assert(!empty());
			return is_small() ? _small.back() : _large.back();
		}

		constexpr const T& back() const
		{
			assert(!empty());
			return is_small() ? _small.back() : _large.back();
		}

		constexpr T * begin()
		{
			return data();
		}

		constexpr const T* begin() const
		{
			return data();
		}

		constexpr T* end()
		{
			return data() + size();
		}

		constexpr const T* end()const
		{
			return data() + size();
		}

		constexpr auto cbegin()const
		{
			return begin();
		}

		constexpr auto cend()const
		{
			return end();
		}

		constexpr size_type max_size()const
		{
			return _large.max_size();
		}

		constexpr void reserve(size_type c)
		{
			_large.reserve(c);
			if constexpr (policy == SmallVectorPolicy::TargetLarge)
			{
				if (c > BaseType::small_capacity() && is_small())
				{
					_large.resize(_small_size);
					moveSmallToLarge();
				}
			}
		}

		constexpr size_type capacity()const
		{
			return std::max(BaseType::small_capacity(), _large.capacity());
		}

		constexpr void shrink_to_fit()
		{
			_large.shrink_to_fit();
		}


		constexpr void clear()
		{
			if constexpr (policy == SmallVectorPolicy::TargetSmall)
			{
				if (is_small())
				{
					clearSmall();
				}
				else
				{
					_large.clear();
				}
				_small_size = 0;
			}
			else
			{
				if (is_small())
				{
					clearSmall();
					_small_size = 0;
				}
				else
				{
					_large.clear();
				}
			}
		}

	protected:

		constexpr void _resize_small(size_type s)
		{
			if (s < _small_size)
			{
				// destroy the objects beyond
				for (size_t i = s; i < BaseType::small_capacity(); ++i)
				{
					if (i < _small_size)
					{
						_small[i].~T();
					}
				}
			}
			else
			{
				// Construct objects beyond?
			}
			_small_size = s;
		}

	public:

		constexpr void resize(size_type s)
		{
			if (s != size())
			{
				if constexpr (policy == SmallVectorPolicy::TargetSmall)
				{
					if(s <= BaseType::small_capacity())
					{
						if (is_small())
						{
							_resize_small(s);
						}
						else
						{
							_large.resize(s);
							moveLargeToSmall();
						}
					}
					else
					{
						_large.resize(s);
						if (is_small())
						{
							moveSmallToLarge();
						}
					}
				}
				else
				{
					if (s <= BaseType::small_capacity() && is_small())
					{
						_resize_small(s);
					}
					else
					{
						_large.resize(s);
						if(is_small())
						{
							moveSmallToLarge();
						}
					}
				}
			}
		}

	protected:

		template <class Q>
		constexpr void _push_back(Q && value)
		{
			if (is_small())
			{
				if ((_small_size + 1) <= BaseType::small_capacity())
				{
					_small[_small_size] = std::forward<Q>(value);
					++_small_size;
				}
				else
				{
					_large.resize(_small_size + 1);
					moveSmallToLarge();
					_large.back() = std::forward<Q>(value);
				}
			}
			else
			{
				_large.push_back(std::forward<Q>(value));
			}
		}

	public:


		constexpr void push_back(const T& value)
		{
			_push_back(value);
		}

		constexpr void push_back(size_type count, const T& value)
		{

		}

		constexpr void push_back(const T&& value)
		{
			_push_back(std::move(value));
		}

		template <class ...Args>
		constexpr T& emplace_back(Args&& ... args)
		{
			T * res = nullptr;
			if (is_small())
			{
				if ((_small_size + 1) <= BaseType::small_capacity())
				{
					_small[_small_size] = T(std::forward<Args>(args)...);
					res = _small.data() + _small_size;
					++_small_size;
					return res;
				}
				else
				{
					_large.reserve(_small_size + 1);
					_large.resize(_small_size);
					moveSmallToLarge();
					res = &_large.emplace_back(std::forward<Args>(args)...);
				}
			}
			else
			{
				res = &_large.emplace_back(std::forward<Args>(args)...);
			}
			assert(res);
			return *res;
		}

		template <std::input_iterator It>
		constexpr void push_back(It first, It last)
		{
			const size_type n = std::distance(first, last);
			if (is_small())
			{
				if ((size_type(_small_size) + n) <= BaseType::small_capacity())
				{
					// TODO
					assert(false);
				}
				else
				{
					_large.reserve(_small_size + n);
					_large.resize(_small_size);
					moveSmallToLarge();
					_large.insert(_large.end(), first, last);
				}
			}
			else
			{
				_large.insert(_large.end(), first, last);
			}
		}

		constexpr void push_back(std::initializer_list<T> ilist)
		{
			push_back(ilist.begin(), ilist.end());
		}


		constexpr void pop_back()
		{
			assert(!empty());
			if (is_small())
			{
				--_small_size;
				_small[_small_size].~T();
			}
			else
			{
				_large.pop_back();
				if (policy == SmallVectorPolicy::TargetSmall)
				{
					if (_large.size() == BaseType::small_capacity())
					{
						moveLargeToSmall();
					}
				}
			}
		}

	protected:

		template <class Q>
		constexpr iterator _insert(const_iterator pos, Q&& value)
		{
			// TODO
			iterator res;
			if (pos == cend())
			{
				_push_back(std::forward<Q>(value));
				res = cend() - 1;
			}
			else
			{
				size_type n = pos - cbegin();
				if (is_small())
				{
					if ((_small_size + 1) <= BaseType::small_capacity())
					{
						++_small_size;

					}
				}
				else
				{
					auto _res = _large.insert(_large.cbegin() + n, std::forward<Q>(value));
					res = _large.data() + n;
				}
			}
			return res;
		}

	public:

		// (1)
		constexpr iterator insert(const_iterator pos, const T& value)
		{
			return _insert(value);
		}

		// (2)
		constexpr iterator insert(const_iterator pos, T&& value)
		{
			return _insert(std::move(value));
		}

		// (3)
		constexpr iterator insert(const_iterator pos, size_type count, const T& value)
		{

		}

		// (4)
		template <std::input_iterator It>
		constexpr iterator insert(const_iterator pos, It first, It last)
		{

		}

		// (5)
		constexpr iterator insert(const_iterator pos, std::initializer_list<T> ilist)
		{

		}


	};
	

	// TODO Add allocator
	template <class T, size_t MinimumSmallSize = 1>
	class OptVectorImpl2 : public SmallVectorBase<T, MinimumSmallSize>
	{
	protected:

		using BaseType = typename SmallVectorBase<T, MinimumSmallSize>;

		size_t _size = 0;
		using Small = std::array<T, BaseType::small_capacity()>;
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
				if (c > BaseType::small_capacity())
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

		static constexpr void swap(OptVectorImpl2& a, OptVectorImpl2& b)
		{
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
			std::swap(a._size, b._size);
		}

		constexpr ~OptVectorImpl2()
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
			return _size <= BaseType::small_capacity();
		}
		

		// (1)
		constexpr OptVectorImpl2() = default;

		// (2)
		// Allocator constructor

		// (3)
		explicit constexpr OptVectorImpl2(size_t count, const T& value) :
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
		explicit constexpr OptVectorImpl2(size_t count) :
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
		constexpr OptVectorImpl2(InputIt first, InputIt const& last) :
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
		constexpr OptVectorImpl2(OptVectorImpl2 const& o) :
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
		constexpr OptVectorImpl2(OptVectorImpl2&& o) noexcept:
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
		constexpr OptVectorImpl2(std::initializer_list<T> init) :
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

		//constexpr OptVectorImpl2(std::initializer_list<T> && init) :
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
			return is_small() ? BaseType::small_capacity() : _large.capacity;
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

		constexpr OptVectorImpl2& operator=(const OptVectorImpl2& o)
		{
			growCapacityIFP(o.size());
			_size = o.size();
			std::copy_n(o.data(), _size, data());
			return *this;
		}

		constexpr OptVectorImpl2& operator=(OptVectorImpl2&& o) noexcept
		{
			swap(*this, o);
			return *this;
		}











	};

	template<class T>
	using OptVector = OptVectorImpl1<T>;

	//static_assert(std::concepts::GenericContainer<OptVector<int>>);
}

namespace std
{
	template <class T, size_t M = 1, vkl::SmallVectorPolicy policy = vkl::SmallVectorPolicy::TargetSmall, class Allocator = std::allocator<T>>
	constexpr void swap(vkl::OptVectorImpl1<T, M, policy, Allocator>& a, vkl::OptVectorImpl1<T, M, policy, Allocator>& b)
	{
		vkl::OptVectorImpl1<T, M, Allocator>::swap(a, b);
	}

	template <class T, size_t M = 1>
	constexpr void swap(vkl::OptVectorImpl2<T, M>& a, vkl::OptVectorImpl2<T, M>& b)
	{
		vkl::OptVectorImpl2<T, M>::swap(a, b);
	}
}