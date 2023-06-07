#pragma once 

#include <algorithm>
#include <stdint.h>
#include <memory>
#include <numeric>
#include <vector>
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

template <class T, class Q>
concept Container = std::is_same<Q, typename T::value_type>::value && requires(T const& t) { t.begin(); t.end(); };

namespace std
{
	template <class T, Container<T> C>
	std::vector<typename C::value_type> makeVector(C const& c)
	{
		return std::vector(c.begin(), c.end());
	}

	template <class T>
	std::vector<T> makeVector(std::vector<T>&& v)
	{
		return std::move(v);
	}

	namespace containers_operators
	{
		template<class T>
		std::vector<T>& operator+=(std::vector<T>& a, T&& b)
		{
			a.emplace_back(std::forward<T>(b));
			return a;
		}

		template<class T, Container<T> Q>
		std::vector<T>& operator+=(std::vector<T>& a, const Q & b)
		{
			a.insert(a.end(), b.begin(), b.end());
			return a;
		}

		// Can't work because can't deduce T...
		//template <class T, Container<T> A, Container<T> B>
		//std::vector<T> operator+(const A& a, const B& b)
		//{
		//	std::vector<T> res = MakeVector(a);
		//	res += b;
		//	return res;
		//}

		template <class T, Container<T> C>
		std::vector<T> operator+(const std::vector<T>& a, const C& b)
		{
			std::vector<T> res = a;
			res += b;
			return res;
		}

		template <class T, Container<T> C>
		std::vector<T> operator+(std::vector<T> && a, const C & b)
		{
			std::vector<T> res = std::move(a);
			res += b;
			return res;
		}

		template <class T, Container<T> C>
		std::vector<T> operator+(const C & a, T && b)
		{
			std::vector<T> res = makeVector(a);
			res += std::forward<T>(b);
			return res;
		}

		template <class T>
		std::vector<T> operator+(const std::vector<T>& a, T&& b)
		{
			std::vector<T> res = a;
			res += std::forward<T>(b);
			return res;
		}

		template <class T>
		std::vector<T> operator+(std::vector<T>&& a, T && b)
		{
			std::vector<T> res = std::move(a);
			res += std::forward<T>(b);
			return res;
		}
	}

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
	Int divCeil(Int a, Int b)
	{
		return (a + b - 1) / b;
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