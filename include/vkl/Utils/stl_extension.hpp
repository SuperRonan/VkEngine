#pragma once 

#include <type_traits>
#include <algorithm>
#include <stdint.h>
#include <memory>
#include <numeric>
#include <vector>
#include <iterator>
#include <string>
#include <string_view>
#include <span>
#include <ranges>
#include <concepts>

#include <that/core/Strings.hpp>

#include <vkl/Utils/Container.hpp>

using namespace std::literals;

namespace std
{
	// D is strictly derived from B (D cannot be B)
	template <class D, class B>
	concept strictly_derived_from = std::derived_from<D, B> && !std::same_as<D, B>;

	//template <class T, Container<T> C>
	//std::vector<typename C::value_type> makeVector(C const& c)
	//{
	//	return std::vector(c.begin(), c.end());
	//}

	//template <class T>
	//std::vector<T> makeVector(std::vector<T>&& v)
	//{
	//	return std::move(v);
	//}

	template <concepts::GenericGrowableContainerMaybeRef Container>
	struct ContainerUseCommonAppendConcatAndOperators : public std::false_type
	{};

	template <concepts::GenericGrowableSetMaybeRef Set>
	struct SetUseCommonOperators : public std::false_type
	{};

	namespace concepts
	{
		template <class C>
		concept GenericContainerNeedingCommonAppendConcatAndOperator = requires
		{
			requires GenericGrowableContainerMaybeRef<C>;
			requires ContainerUseCommonAppendConcatAndOperators<typename std::remove_reference<C>::type>::value;
		};

		template <class C, class T>
		concept ContainerNeedingCommonAppendConcatAndOperator = requires
		{
			requires GrowableContainerMaybeRef<C, T>;
			requires ContainerUseCommonAppendConcatAndOperators<typename std::remove_reference<C>::type>::value;
		};

		template <class S>
		concept GenericSetNeedingCommonOperators = requires
		{
			requires GenericGrowableSetMaybeRef<S>;
			requires SetUseCommonOperators<typename std::remove_reference<S>::type>::value;
		};

		template <class S, class T>
		concept SetNeedingCommonOperators = requires
		{
			requires GrowableSetMaybeRef<S, T>;
			requires SetUseCommonOperators<typename std::remove_reference<S>::type>::value;
		};
	}

	template<class T, concepts::ContainerNeedingCommonAppendConcatAndOperator<T> C>
	C& append(C& c, T&& b)
	{
		c.push_back(std::forward<T>(b));
		return c;
	}

	template <concepts::GenericContainerNeedingCommonAppendConcatAndOperator C, concepts::ConvertibleContainerMaybeRef<typename std::remove_reference<C>::type> CC>
	C& append(C& a, CC const& b)
	{
		std::copy(b.begin(), b.end(), std::back_inserter(a));
		return a;
	}

	template <class T, concepts::ContainerNeedingCommonAppendConcatAndOperator<T> C>
	C concat(C && c, T&& t)
	{
		C res = std::forward<C>(c);
		append(res, std::forward<T>(t));
		return res;
	}

	template <concepts::GenericContainerNeedingCommonAppendConcatAndOperator C, concepts::ConvertibleContainerMaybeRef<typename std::remove_reference<C>::type> CC>
	C concat(C && c, CC const& cc)
	{
		C res = std::forward<C>(c);
		append(res, cc);
		return res;
	}
}


namespace std
{
	namespace containers_append_operators
	{
		template<class T, ::std::concepts::ContainerNeedingCommonAppendConcatAndOperator<T> C>
		C& operator+=(C& c, T&& b)
		{
			return std::append(c, std::forward<T>(b));
		}

		template<class T, ::std::concepts::ContainerNeedingCommonAppendConcatAndOperator<T> C>
		C& operator|=(C& c, T&& b)
		{
			return c+= std::forward<T>(b);
		}

		template <::std::concepts::GenericContainerNeedingCommonAppendConcatAndOperator C, ::std::concepts::ConvertibleContainerMaybeRef<typename std::remove_reference<C>::type> CC>
		C& operator+=(C& a, CC const& b)
		{
			return std::append(a, b);
		}

		template <::std::concepts::GenericContainerNeedingCommonAppendConcatAndOperator C, ::std::concepts::ConvertibleContainerMaybeRef<typename std::remove_reference<C>::type> CC>
		C& operator|=(C& a, CC const& b)
		{
			return a += b;
		}

		template <class T, ::std::concepts::ContainerNeedingCommonAppendConcatAndOperator<T> C>
		C operator+(C&& c, T&& t)
		{
			return std::concat(std::forward<C>(c), std::forward<T>(t));
		}

		template <class T, ::std::concepts::ContainerNeedingCommonAppendConcatAndOperator<T> C>
		C operator|(C&& c, T&& t)
		{
			return std::forward<C>(c) + std::forward<T>(t);
		}

		template <::std::concepts::GenericContainerNeedingCommonAppendConcatAndOperator C, ::std::concepts::ConvertibleContainerMaybeRef<typename std::remove_reference<C>::type> CC>
		C operator+(C&& c, CC const& cc)
		{
			return std::concat(std::forward<C>(c), cc);
		}

		template <::std::concepts::GenericContainerNeedingCommonAppendConcatAndOperator C, ::std::concepts::ConvertibleContainerMaybeRef<typename std::remove_reference<C>::type> CC>
		C operator|(C&& c, CC const& cc)
		{
			return std::forward<C>(c) + cc;
		}




		template <::std::concepts::GenericSetNeedingCommonOperators S, std::convertible_to<typename S::value_type> T>
		S& operator|=(S& s, T&& t)
		{
			s.insert(std::forward<T>(t));
			return s;
		}

		template <::std::concepts::GenericSetNeedingCommonOperators S, std::convertible_to<typename S::value_type> T>
		S operator|(S&& s, T&& t)
		{
			S res = std::forward<S>(s);
			res |= std::forward<T>(t);
			return res;
		}

		template <::std::concepts::GenericSetNeedingCommonOperators S, ::std::concepts::ConvertibleSetMaybeRef<typename std::remove_reference<S>::type> Q>
		S& operator|=(S& s, Q&& q)
		{
			s.insert(q.begin(), q.end());
			return s;
		}

		template <::std::concepts::GenericSetNeedingCommonOperators S, ::std::concepts::ConvertibleSetMaybeRef<typename std::remove_reference<S>::type> Q>
		S operator|(S&& s, Q&& q)
		{
			S res = std::forward<S>(s);
			res |= std::forward<Q>(q);
			return res;
		}

		//template<class T>
		//std::vector<T>& operator+=(std::vector<T>& a, T&& b)
		//{
		//	a.emplace_back(std::forward<T>(b));
		//	return a;
		//}

		//template<class T, Container<T> Q>
		//std::vector<T>& operator+=(std::vector<T>& a, const Q & b)
		//{
		//	a.insert(a.end(), b.begin(), b.end());
		//	return a;
		//}

		// Can't work because can't deduce T...
		//template <class T, Container<T> A, Container<T> B>
		//std::vector<T> operator+(const A& a, const B& b)
		//{
		//	std::vector<T> res = MakeVector(a);
		//	res += b;
		//	return res;
		//}

		//template <class T, Container<T> C>
		//std::vector<T> operator+(const std::vector<T>& a, const C& b)
		//{
		//	std::vector<T> res = a;
		//	res += b;
		//	return res;
		//}

		//template <class T, Container<T> C>
		//std::vector<T> operator+(const C& b, const std::vector<T>& a)
		//{
		//	std::vector<T> res = makeVector(b);
		//	res += a;
		//	return res;
		//}

		//// Have to declare this one, else it would be ambiguous
		//template <class T>
		//std::vector<T> operator+(std::vector<T> const& a, std::vector<T> const& b)
		//{
		//	std::vector<T> res = a;
		//	res += b;
		//	return res;
		//}

		//template <class T, Container<T> C>
		//std::vector<T> operator+(std::vector<T> && a, const C & b)
		//{
		//	std::vector<T> res = std::move(a);
		//	res += b;
		//	return res;
		//}

		//template <class T, Container<T> C>
		//std::vector<T> operator+(const C & a, T && b)
		//{
		//	std::vector<T> res = makeVector(a);
		//	res += std::forward<T>(b);
		//	return res;
		//}

		//template <class T>
		//std::vector<T> operator+(const std::vector<T>& a, T&& b)
		//{
		//	std::vector<T> res = a;
		//	res += std::forward<T>(b);
		//	return res;
		//}

		//template <class T>
		//std::vector<T> operator+(std::vector<T>&& a, T && b)
		//{
		//	std::vector<T> res = std::move(a);
		//	res += std::forward<T>(b);
		//	return res;
		//}
	}
}

namespace std
{
	namespace concepts
	{
		template <class T>
		concept HashableFromMethod = requires(T const& t)
		{
			{ t.hash() } -> std::same_as<size_t>; 
		};
	}

	template <concepts::HashableFromMethod T>
	struct hash<T>
	{
		constexpr size_t operator()(T const& t)const
		{
			return t.hash();
		}
	};

	template <class T>
	constexpr T& zeroInit(T& t)
	{
		std::memset(&t, 0, sizeof(T));
		return t;
	}


	template <class It, class Rate>
	It findBest(It begin, const It& end, Rate const& rate)
	{
		auto res = begin;
		auto best_rate = rate(*res);
		++begin;
		for (; begin != end; ++begin)
		{
			auto new_rate = rate(*begin);
			if (new_rate > best_rate)
			{
				best_rate = new_rate;
				res = begin;
			}
		}
		return res;
	}

	template <class Object>
	constexpr void rawCopySwap(Object& a, Object& b)
	{
		std::array<int8_t, sizeof(Object)> tmp;
		std::copy((int8_t*)&a, (int8_t*)&a + sizeof(Object), tmp.data()); // tmp = a
		std::copy((int8_t*)&b, (int8_t*)&b + sizeof(Object), (int8_t*)&a); // a = b
		std::copy(tmp.data(), tmp.data() + sizeof(Object), (int8_t*)&b); // b = tmp
	}

	template <class It1, class It2, class It3, class T>
	constexpr decltype(auto) inner(It1 begin1, It2 begin2, It3 const& end1, T const& acc)
	{
		while (begin1 != end1)
		{
			acc += *begin1 * *begin2;
			++begin1;
			++begin2;
		}
		return acc;
	}

	template<class Int>
	constexpr Int divCeil(Int a, Int b)
	{
		return (a + b - 1) / b;
	}

	template <class T>
	constexpr auto sqr(T const& t)
	{
		return t * t;
	}

	template <class T>
	std::vector<T> filterRedundantValues(std::vector<T> const& vec)
	{
		std::vector<T> res;
		for (auto const& elem : vec)
		{
			if (std::find(res.cbegin(), res.cend(), elem) == res.cend())
			{
				res.push_back(elem);
			}
		}
		return res;
	}

	template <class It1, class It2, class T>
	bool contains(It1 begin, It2 const& end, T const& value)
	{
		return std::find(begin, end, value) != end;
	}

	template <class T, class Q = T>
	bool contains(std::vector<T> const& vec, Q const& value)
	{
		return std::contains(vec.cbegin(), vec.cend(), value);
	}

	// TODO C++23 (std::string_view with implement this natively as a member function)
	template <class Char>
	bool contains_sv(const std::basic_string_view<Char>& hay, const std::basic_string_view<Char>& needle)
	{
		return hay.find(needle) != std::basic_string_view<Char>::npos;
	}

	template <that::concepts::GenericString StrHay, that::concepts::BasicStringLike<that::GenericStringCharType<StrHay>> StrNeedle>
	bool contains(const StrHay& hay, const StrNeedle& needle)
	{
		using Char = that::GenericStringCharType<StrHay>;
		using SV = std::basic_string_view<Char>;
		return contains_sv<Char>(SV(hay), SV(needle));
	}

	template <class Char>
	bool containsCaseInsensitive_sv(const std::basic_string_view<Char>& hay, const std::basic_string_view<Char>& needle)
	{
		return std::search(
			hay.cbegin(), hay.cend(),
			needle.cbegin(), needle.cend(),
			[](Char a, Char b) {return std::tolower(a) == std::tolower(b); }
		) != hay.cend();
	}

	template <that::concepts::GenericString StrHay, that::concepts::BasicStringLike<that::GenericStringCharType<StrHay>> StrNeedle>
	bool containsCaseInsensitive(const StrHay& hay, const StrNeedle& needle)
	{
		using Char = that::GenericStringCharType<StrHay>;
		using SV = std::basic_string_view<Char>;
		return containsCaseInsensitive_sv<Char>(SV(hay), SV(needle));
	}



	template <std::forward_iterator It>
	size_t HashSequence(It const& begin, It const& end) noexcept
	{
		auto it = begin;
		size_t res = 0;
		std::hash<typename It::value_type> hasher = {};
		while (it != end)
		{
			res ^= hasher(*it++);
		}
		return res;
	}

	template <ranges::input_range Range>
	size_t HashRange(Range const& r) noexcept
	{
		return HashSequence(r.begin(), r.end());
	}


	template <class C, template <class> class ValidRet, class ...Args>
	concept invocable_valid_returns = std::invocable<C, Args...> && ValidRet<typename std::invoke_result<C, Args...>::type>::value;

	template <template <class, class> class Bin, class Second>
	struct bind_second
	{
		template <class First>
		using type = Bin<First, Second>;
	};

	template <template <class, class> class Bin, class First>
	struct bind_first
	{
		template <class Second>
		using type = Bin<First, Second>;
	};

	template <class C, class R, class ...Args>
	concept invocable_compatible_returns = invocable_valid_returns<C, bind_second<std::is_convertible, R>::template type, Args...>;

	namespace ranges
	{
		// std::ranges does not provide std::ranges::lexicographical_compare_three_way()
		template <input_range _Rng1, input_range _Rng2, class _Pj1 = identity, class _Pj2 = identity,
			std::invocable<typename projected<iterator_t<_Rng1>, _Pj1>::value_type, typename projected<iterator_t<_Rng2>, _Pj2>::value_type> _Pr = std::compare_three_way>
		constexpr auto lexicographical_compare_three_way(_Rng1&& _Range1, _Rng2&& _Range2, _Pr _Pred = {}, _Pj1 _Proj1 = {}, _Pj2 _Proj2 = {})
		{
			return ::std::lexicographical_compare_three_way(
				ranges::begin(_Range1), ranges::end(_Range1),
				ranges::begin(_Range2), ranges::end(_Range2),
				[&](auto&& lhs, auto&& rhs) {
					return _Pred(std::invoke(_Proj1, lhs), std::invoke(_Proj2, rhs));
				}
			);
		}

		// First compare the sizes of the operands, than a lixicographical comp if same size
		template <sized_range _Rng1, sized_range _Rng2, class _Pj1 = identity, class _Pj2 = identity,
			std::invocable_compatible_returns<std::strong_ordering, typename projected<iterator_t<_Rng1>, _Pj1>::value_type, typename projected<iterator_t<_Rng2>, _Pj2>::value_type> _Pr = std::compare_three_way>
		constexpr std::strong_ordering compare_three_way_size_lexicographical(_Rng1&& _Range1, _Rng2&& _Range2, _Pr _Pred = {}, _Pj1 _Proj1 = {}, _Pj2 _Proj2 = {})
		{
			const std::strong_ordering size_comp = ranges::size(_Range1) <=> ranges::size(_Range2);
			if (size_comp != std::strong_ordering::equal)
			{
				return size_comp;
			}
			return lexicographical_compare_three_way(std::forward<_Rng1>(_Range1), std::forward<_Rng2>(_Range2), _Pred, _Proj1, _Proj2);
		}
	}
}