#pragma once

#include <that/utils/EnumClassOperators.hpp>
#include <functional>
#include <ostream>
#include <mutex>

namespace vkl
{
	struct Logger
	{
		enum class Options : uint
		{
			None = 0,
			VerbosityMostImportant = 0, 
			VerbosityImportant = 1,
			VerbosityMedium = 2,
			VerbosityLeastImportant = 3, 
			MaxVerbosity = VerbosityLeastImportant,
			_VerbosityMask = std::bitMask<uint>(std::bit_width(static_cast<uint>(MaxVerbosity))),
			_TagBitOffset = 2,
			_TagBitCount = 4,
			TagNone = (0 << _TagBitOffset),
			TagInfo1 = (1 << _TagBitOffset), // cyan
			TagInfo2 = (2 << _TagBitOffset), // blue
			TagInfo3 = (3 << _TagBitOffset), // magenta
			TagInfo = TagInfo1,
			TagSuccess = (4 << _TagBitOffset), // green
			TagLowWarning = (5 << _TagBitOffset), // yellow
			TagWarning = (6 << _TagBitOffset), // yellow
			TagHighWarning = (7 << _TagBitOffset), // bright yellow 
			TagError = (8 << _TagBitOffset), // red
			TagFatalError = (9 << _TagBitOffset), // brigh red
			_TagMask = std::bitMask<uint>(_TagBitCount) << _TagBitOffset,
			NoEndL = 1 << (_TagBitOffset + _TagBitCount),
			NoTime = NoEndL << 1,
			NoLock = NoTime << 1,
		};

		uint max_verbosity = 0;
		using LoggingFunction = std::function<void(std::string_view, Options)>;
		LoggingFunction log_f = {};

		bool canLog(uint verbosity) const
		{
			return (verbosity <= max_verbosity) && !!log_f;
		}

		void log(std::string_view sv, Options options) const;

		void log(std::string_view sv) const
		{
			return log(sv, Options::None);
		}

		void operator()(std::string_view sv, Options options)const
		{
			return log(sv, options);
		}

		void operator()(std::string_view sv) const
		{
			return log(sv);
		}
	};

	THAT_DECLARE_ENUM_CLASS_OPERATORS(Logger::Options)
}