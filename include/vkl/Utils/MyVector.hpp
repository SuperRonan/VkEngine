#pragma once

#include <vector>
#include <iterator>
#include <utility>

#include <vkl/Utils/Container.hpp>

namespace vkl
{
	template <class T, class Allocator = std::allocator<T>>
	class MyVector final : public std::vector<T, Allocator>
	{ 
	protected:


	public:

		using ParentType = std::vector<T, Allocator>;
		using size_type = typename ParentType::size_type;
		
		// (1)
		constexpr MyVector() noexcept(noexcept(Allocator())) = default;

		// (2)
		constexpr explicit MyVector(const Allocator& alloc) noexcept :
			ParentType(alloc)
		{}

		// (3)
		constexpr MyVector(size_type count, const T& value, const Allocator & alloc = Allocator()) :
			ParentType(count, value, alloc)
		{}

		// (4)
		constexpr explicit MyVector(size_type count, const Allocator& alloc = Allocator()) :
			ParentType(count, alloc)
		{}

		// (5)
		template <std::input_iterator InputIt>
		constexpr MyVector(InputIt first, InputIt last, const Allocator& alloc = Allocator()) :
			ParentType(first, last, alloc)
		{}

		// (6)
		constexpr MyVector(MyVector const& other) :
			ParentType(other)
		{}

		constexpr MyVector(std::vector<T, Allocator> const& other) : 
			ParentType(other)
		{}

		// (7)
		constexpr MyVector(MyVector const& other, const Allocator& alloc) :
			ParentType(other, alloc)
		{}

		// (8)
		constexpr MyVector(MyVector && other) noexcept :
			ParentType(std::move(other))
		{}

		constexpr MyVector(std::vector<T, Allocator>&& other) noexcept :
			ParentType(std::move(other))
		{}

		// (9)
		constexpr MyVector(MyVector&& other, const Allocator& alloc) noexcept :
			ParentType(std::move(other), alloc)
		{}

		// (10)
		constexpr MyVector(std::initializer_list<T> init, const Allocator& alloc = Allocator()) :
			ParentType(init, alloc)
		{}

		// (11) Range C++ 23

		
		
		constexpr ~MyVector() = default;


		// (1)
		constexpr MyVector& operator=(const MyVector& other)
		{
			ParentType::operator=(other);
			return *this;
		}

		constexpr MyVector& operator=(const std::vector<T, Allocator> & other)
		{
			ParentType::operator=(other);
			return *this;
		}

		// (2)
		constexpr MyVector& operator=(MyVector&& other) noexcept
		{
			ParentType::operator=(std::move(other));
			return *this;
		}

		constexpr MyVector& operator=(const std::vector<T, Allocator> && other)
		{
			ParentType::operator=(std::move(other));
			return *this;
		}

		// (3)
		constexpr MyVector& operator=(std::initializer_list<T> ilist)
		{
			ParentType::operator=(ilist);
			return *this;
		}


		constexpr operator bool()const
		{
			return !ParentType::empty();
		}

		constexpr uint32_t size32() const
		{
			return static_cast<uint32_t>(ParentType::size());
		}

		constexpr size_t byte_size()const
		{
			return sizeof(T) * ParentType::size();
		}

		MyVector& operator+=(T const& t)
		{
			ParentType::push_back(t);
			return *this;
		}

		MyVector& operator+=(T && t)
		{
			ParentType::push_back(std::forward<T>(t));
			return *this;
		}

		template <std::concepts::ContainerMaybeRef<T> C>
		MyVector& operator+=(C const& c)
		{
			ParentType::insert(ParentType::end(), c.begin(), c.end());
			return *this;
		}

		MyVector operator+(T const& t) const
		{
			MyVector res = *this;
			res += t;
			return res;
		}

		MyVector operator+(T && t) const
		{
			MyVector res = *this;
			res += std::forward<T>(t);
			return res;
		}

		template <std::concepts::ContainerMaybeRef<T> C>
		MyVector operator+(C const& c) const
		{
			MyVector res = *this;
			res += c;
			return res;
		}

	};

	static_assert(std::concepts::GenericContainer<MyVector<int>>);
}