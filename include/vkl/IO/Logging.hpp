#pragma once

#include <ostream>
#include <chrono>
#include <string_view>
#include <vkl/Core/LogOptions.hpp>

namespace vkl
{

	struct TagStr
	{
		std::string_view token = {};
		std::string_view openning = {};
		std::string_view closing = {};
	};

	extern TagStr GetTagStr(Logger::Options tag);

	template <class Stream, class Duration>
	static Stream& LogDurationAsTimePoint(Stream& stream, Duration const& d)
	{
		const auto prev_precision = stream.precision();
		const auto prev_fill = stream.fill();
		const auto prev_w = stream.width();
		stream << '[';
		stream << std::setfill('0');
		std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(d);
		stream << h.count() << "h ";
		std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(d);
		stream << std::setw(2) << m.count() % 60 << "m ";
		std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(d);
		stream << std::setw(2) << std::setfill(' ') << s.count() % 60 << ".";
		std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(d);
		stream << std::setw(3) << ms.count() % 1000;
		stream << "s]";
		stream << std::setprecision(prev_precision);
		stream << std::setfill(prev_fill);
		stream << std::setw(prev_w);
		return stream;
	}

}