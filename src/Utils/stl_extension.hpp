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

namespace std
{
	template <class T>
	constexpr T& zeroInit(T& t)
	{
		std::memset(&t, 0, sizeof(T));
		return t;
	}


	template <class It, class Rate>
	It find_best(It begin, const It& end, Rate const& rate)
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
}