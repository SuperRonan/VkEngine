#include <vkl/Core/LogOptions.hpp>
#include <iostream>

namespace vkl
{
	void Logger::log(std::string_view sv, Options options) const
	{
		const uint verbosity = static_cast<uint>(options & Options::_VerbosityMask);
		if (canLog(verbosity))
		{
			log_f(sv, options);
		}
	}
}