#pragma once 

#include <type_traits>
#include <algorithm>
#include <stdint.h>
#include <memory>
#include <numeric>
#include <vector>
#include <iterator>
#include <string>

#include <vkl/Utils/Container.hpp>

#include <glm/glm.hpp>

using namespace std::literals;

namespace glm_operators
{
	template <class Stream, class Float, glm::length_t N, glm::qualifier q>
	Stream& operator<<(Stream& stream, glm::vec<N, Float, q> const& vec)
	{
		stream << "[";
		for (int i = 0; i < N; ++i)
		{
			stream << vec[i] << ", ";
		}
		stream << "]";
		return stream;
	}

	template <class Stream, class Float, glm::length_t N, glm::length_t M, glm::qualifier q>
	Stream& operator<<(Stream& stream, glm::mat<N, M, Float, q> const& mat)
	{
		stream << "[";
		for (int i = 0; i < N; ++i)
		{
			stream << mat[i] << ", ";
		}
		stream << "]";
		return stream;
	}
}

namespace glm
{
	template<class Float, glm::length_t N, qualifier q>
	std::string toString(vec<N, Float, q> const& vec)
	{
		//using namespace glm_operators;
		std::stringstream ss;
		//ss << vec;
		glm_operators::operator<<(ss, vec);
		return ss.str();
	}

	template <class Float, glm::length_t N, glm::length_t M, qualifier q>
	std::string toString(mat<N, M, Float, q> const& mat)
	{
		//using namespace glm_operators;
		std::stringstream ss;
		glm_operators::operator<<(ss, mat);
		return ss.str();
	}
}

namespace std
{
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
}	