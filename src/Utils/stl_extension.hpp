#pragma once 

#include <algorithm>
#include <stdint.h>
#include <memory>
#include <numeric>
#include <vector>
#include <glm/glm.hpp>

template <class Stream, class Float, glm::length_t N>
Stream& operator<<(Stream& stream, glm::vec<N, Float> const& vec)
{
	stream << "[";
	for (int i = 0; i < N; ++i)
	{
		stream << vec[i] << ", ";
	}
	stream << "]";
	return stream;
}

template <class Stream, class Float, glm::length_t N, glm::length_t M>
Stream& operator<<(Stream& stream, glm::mat<N, M, Float> const& mat)
{
	stream << "[";
	for (int i = 0; i < N; ++i)
	{	
		stream << mat[i] << ", ";
	}
	stream << "]";
	return stream;
}

namespace std_vector_operators
{
	template<class T>
	std::vector<T>& operator+=(std::vector<T>& a, T&& b)
	{
		a.emplace_back(std::forward<T>(b));
		return a;
	}

	template<class T>
	std::vector<T>& operator+=(std::vector<T>& a, std::vector<T>&& b)
	{
		a.insert(a.end(), b.cbegin(), b.cend());
		return a;
	}

	template <class T>
	std::vector<T> operator+(std::vector<T>&& a, std::vector<T>&& b)
	{
		std::vector<T> res = std::forward<std::vector<T>>(a);
		res += b;
		return res;
	}
}

namespace std
{
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
			if (new_rate < best_rate)
			{
				best_rate = new_rate;
				res = begin;
			}
		}
		return res;
	}

	template <class Object>
	constexpr void copySwap(Object& a, Object& b)
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
}