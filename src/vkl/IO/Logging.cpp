#include <vkl/IO/Logging.hpp>

#include <array>

namespace vkl
{
#if _WINDOWS && 0
	// Does not work yet
	static const std::array<std::string_view, 4> tags = {"ℹ️", "⚠️", "❌", "🛑"};
#else
	static const std::array<std::string_view, 4> tags = {"[i]", "/!\\", "[X]", "[XX]"};
#endif

	static const std::array<std::string_view, 4> colors = {
		"\033[36m",
		"\033[33m",
		"\033[31m",
		"\033[35m",
	};

	TagStr GetTagStr(Logger::Options tag)
	{
		TagStr res;
		if (tag != Logger::Options::TagNone)
		{
			uint index = static_cast<uint>(tag - Logger::Options::TagInfo);
			index = std::max<uint>(index, tags.size());
			res.token = tags[index];
			res.openning = colors[index];
			res.closing = "\033[0m";
		}

		return res;
	}
}