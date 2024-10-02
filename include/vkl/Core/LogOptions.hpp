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
			VerbosityMask = std::bitMask<uint>(std::bit_width(static_cast<uint>(MaxVerbosity))),
			TagNone = 0,
			TagInfo = MaxVerbosity + 1,
			TagWarning = TagInfo + 1,
			TagError = TagWarning + 1,
			TagFatalError = TagError + 1,
			TagMask = 0b11100,
			NoEndL = 0b100000,
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