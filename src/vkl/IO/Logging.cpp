#include <vkl/IO/Logging.hpp>

#include <array>

namespace vkl
{
#if _WINDOWS && 0
	// Does not work yet
	static const std::array<std::string_view, 9> tags = {
		"ℹ️", 
		"ℹ️", 
		"ℹ️", 
		"✅", 
		"⚠️", 
		"⚠️", 
		"⚠️", 
		"❌", 
		"🛑"
	};
#else
	static const std::array<std::string_view, 9> tags = {
		"[i]", 
		"[i]", 
		"[i]", 
		
		"[v]",
		
		"/!\\", 
		"/!!\\", 
		"/!!!\\", 
		
		"[X]", 
		"[XX]"
	};
#endif
// https://en.wikipedia.org/wiki/ANSI_escape_code

	
#define ANSI_Black 30
#define ANSI_Red 31
#define ANSI_Green 32
#define ANSI_Yellow 33
#define ANSI_Blue 34
#define ANSI_Magenta 35
#define ANSI_Cyan 36
#define ANSI_White 37

#define ANSI_Bright_Black 90
#define ANSI_Bright_Red 91
#define ANSI_Bright_Green 92
#define ANSI_Bright_Yellow 93
#define ANSI_Bright_Blue 94
#define ANSI_Bright_Magenta 95
#define ANSI_Bright_Cyan 96
#define ANSI_Bright_White 97
	
#define MAKE_STREAM_ANSI_COLOR2(color) "\x1B[" #color "m"
#define MAKE_STREAM_ANSI_COLOR(color) MAKE_STREAM_ANSI_COLOR2(color)

	static const std::array<std::string_view, 9> colors = {
		MAKE_STREAM_ANSI_COLOR(ANSI_Cyan),
		MAKE_STREAM_ANSI_COLOR(ANSI_Blue),
		MAKE_STREAM_ANSI_COLOR(ANSI_Magenta),

		MAKE_STREAM_ANSI_COLOR(ANSI_Green),

		MAKE_STREAM_ANSI_COLOR(ANSI_Yellow),
		MAKE_STREAM_ANSI_COLOR(ANSI_Yellow),
		MAKE_STREAM_ANSI_COLOR(ANSI_Bright_Yellow),
		
		MAKE_STREAM_ANSI_COLOR(ANSI_Red),
		MAKE_STREAM_ANSI_COLOR(ANSI_Bright_Red),
	};

	static const std::array<std::string_view, 4> emph_colors = {
		MAKE_STREAM_ANSI_COLOR(ANSI_Bright_Cyan),
		MAKE_STREAM_ANSI_COLOR(ANSI_Bright_Blue),
		MAKE_STREAM_ANSI_COLOR(ANSI_Bright_Magenta),

		MAKE_STREAM_ANSI_COLOR(ANSI_Bright_Green),
	};

	TagStr GetTagStr(Logger::Options options)
	{
		TagStr res;
		const Logger::Options tag = options & Logger::Options::_TagMask;
		const Logger::Options v = options & Logger::Options::_VerbosityMask;
		const bool v_emphasis = v < Logger::Options::VerbosityMedium;
		if (tag != Logger::Options::TagNone)
		{
			uint index = static_cast<uint>(tag >> Logger::Options::_TagBitOffset) - 1;
			index = std::min<uint>(index, colors.size() - 1);
			if (v_emphasis && index <= 3)
			{
				res.openning = emph_colors[index];
			}
			else
			{
				res.openning = colors[index];
			}
			res.closing = "\033[0m";//MAKE_STREAM_ANSI_COLOR(0);
			res.token = tags[index];
		}

		return res;
	}
}