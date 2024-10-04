#include <vkl/Execution/MessagePopUp.hpp>
#include <vkl/Core/LogOptions.hpp>
#include <vkl/VkObjects/VkWindow.hpp>

#include <algorithm>
#include <iostream>
#include <cassert>

#include <SDL2/SDL_syswm.h>

#define MESSAGE_POPUP_USE_STANDALONE 0
#define MESSAGE_POPUP_USE_SDL 1
#define MESSAGE_POPUP_USE_WINDOWS 2

#if defined SDL_h_ && 1
#define MESSAGE_POPUP_POLICY MESSAGE_POPUP_USE_SDL
#elif _WINDOWS
#define MESSAGE_POPUP_POLICY MESSAGE_POPUP_USE_WINDOWS
#endif

#ifndef MESSAGE_POPUP_POLICY
#define MESSAGE_POPUP_POLICY MESSAGE_POPUP_USE_STANDALONE 
#endif

namespace vkl
{

#ifdef _WINDOWS
	static const std::array<UINT, static_cast<size_t>(MessagePopUp::Type::MAX_VALUE)> Windows_Types = {
		MB_ICONERROR,
		MB_ICONSTOP,
		MB_ICONHAND,
		MB_ICONQUESTION,
		MB_ICONWARNING,
		MB_ICONEXCLAMATION,
		MB_ICONINFORMATION,
		MB_ICONASTERISK,
	};

	static UINT GetWindowsType(MessagePopUp::Type type)
	{
		const uint i = static_cast<uint>(type);
		assert(i < Windows_Types.size());
		return Windows_Types[i];
	}

	static UINT GetWindowsID(MessagePopUp::Button button)
	{
		UINT res = static_cast<UINT>(button) + 1;
		return res;
	}

	static MessagePopUp::Button GetButtonFromWindowsID(UINT wid)
	{
		return static_cast<MessagePopUp::Button>(wid - 1);
	}
#endif

	static const std::array<std::string_view, static_cast<size_t>(MessagePopUp::Button::MAX_VALUE)> Sdl_Button_Labels = {
		"Ok",
		"Cancel",
		"Abort",
		"Retry",
		"Ignore",
		"Yes",
		"No",
		"Close",
		"Help",
		"TryAgain",
		"Continue",
	};

#if MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_WINDOWS
	static UINT GetButtonsFlagsWindows(std::vector<MessagePopUp::Button> const& buttons)
	{
		using Button = MessagePopUp::Button;
		UINT res = 0;
		std::vector<Button> sorted_buttons = buttons;
		std::sort(sorted_buttons.begin(), sorted_buttons.end());

		if (sorted_buttons.size() == 1)
		{
			const Button b0 = sorted_buttons[0];
			if (b0 == Button::Ok)
			{	
				res = MB_OK;
			}
			if (b0 == Button::Help)
			{
				res = MB_HELP;
			}
		}
		else if (sorted_buttons.size() == 2)
		{
			const Button b0 = sorted_buttons[0];
			const Button b1 = sorted_buttons[1];
			if (b0 == Button::Ok && b1 == Button::Cancel)
			{
				res = MB_OKCANCEL;
			}
			else if (b0 == Button::Cancel && b1 == Button::Retry)
			{
				res = MB_RETRYCANCEL;
			}
			else if (b0 == Button::Yes && b1 == Button::No)
			{
				res = MB_YESNO;
			}
		}
		else if (sorted_buttons.size() == 3)
		{
			const Button b0 = sorted_buttons[0];
			const Button b1 = sorted_buttons[1];
			const Button b2 = sorted_buttons[2];
			if (b0 == Button::Abort && b1 == Button::Retry && b2 == Button::Ignore)
			{
				res = MB_ABORTRETRYIGNORE;
			}
			else if(b0 == Button::Cancel && b1 == Button::Retry && b2 == Button::Continue)
			{
				res = MB_CANCELTRYCONTINUE;
			}
			else if (b0 == Button::Cancel && b1 == Button::Yes && b2 == Button::No)
			{
				return MB_YESNOCANCEL;
			}
		}
		return res;
	}
#endif

	MessagePopUp::MessagePopUp(CreateInfo const& ci):
		_type(ci.type),
		_title(ci.title),
		_message(ci.message),
		_buttons(ci.buttons),
		_beep(ci.beep),
		_logger(ci.logger),
		_parent_window(ci.parent_window)
	{}

	static std::array<Logger::Options, 8> Logger_Tags = 
	{
		Logger::Options::TagError,
		Logger::Options::TagError,
		Logger::Options::TagError,

		Logger::Options::TagInfo2,

		Logger::Options::TagWarning,
		Logger::Options::TagLowWarning,

		Logger::Options::TagInfo1,
		Logger::Options::TagInfo3,
	};

	MessagePopUp::Button SynchMessagePopUp::operator()(bool lock_mutex)const
	{
		if (lock_mutex)
		{
			g_common_mutex.lock();
		}
		if (_beep)
		{
#if (MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_WINDOWS) || (defined _WINDOWS)
			MessageBeep(GetWindowsType(_type));
#endif
		}
		Button res;
		if (_logger)
		{
			Logger::Options tag = Logger_Tags[static_cast<uint>(_type)];
			Logger::Options options = Logger::Options::VerbosityMostImportant;
			options |= tag;
			_logger->log(std::format("{}\n{}", _title, _message), options);
		}
#if MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_WINDOWS
		

		HWND owner = nullptr;
		if (_parent_window)
		{
			SDL_Window * sdl_window = _parent_window->handle();
			SDL_SysWMinfo info;
			SDL_VERSION(&info.version);
			SDL_GetWindowWMInfo(sdl_window, &info);
			owner = info.info.win.window;
		}
		
		UINT flags = GetWindowsType(_type) | GetButtonsFlagsWindows(_buttons);
		flags |= MB_SETFOREGROUND; // MB_SETFOREGROUND or MB_TOPMOST appears to do the same
		int ires = MessageBoxA(owner, _message.c_str(), _title.c_str(), flags);
		res = GetButtonFromWindowsID(ires);
#elif MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_SDL
		SDL_MessageBoxData data = {};
		std::memset(&data, 0, sizeof(data));
		data.window = _parent_window ? _parent_window->handle() : nullptr;
		data.title = _title.c_str();
		data.message = _message.c_str();

		data.flags = SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT;
		switch(_type)
		{
			case Type::Error:
			case Type::Stop:
			case Type::Hand:
				data.flags |= SDL_MESSAGEBOX_ERROR;
			break;
			case Type::Question:
				data.flags |= SDL_MESSAGEBOX_QUESTION;
			break;
			case Type::Warning:
			case Type::Exclamation:
				data.flags |= SDL_MESSAGEBOX_WARNING;
			break;
			case Type::Information:
			case Type::Asterisk:
				data.flags |= SDL_MESSAGEBOX_INFORMATION;
			break;
		};

		MyVector<SDL_MessageBoxButtonData> sdl_buttons_storage(_buttons.size());
		data.numbuttons = static_cast<int>(_buttons.size());
		SDL_MessageBoxButtonData * sdl_buttons = sdl_buttons_storage.data();


		data.buttons = sdl_buttons;
		for (size_t i = 0; i < static_cast<size_t>(data.numbuttons); ++i)
		{
			SDL_MessageBoxButtonData & b = sdl_buttons[i];
			b.buttonid = static_cast<int>(_buttons[i]);
			b.text = Sdl_Button_Labels[b.buttonid].data();
			b.flags = 0;
			if(i == 0)
			{
				b.flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
			}
			if((i + 1) == static_cast<size_t>(data.numbuttons))
			{
				b.flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
			}
		}

		data.colorScheme = nullptr;
		res = _buttons[0];
		int res_id;
		const int sdl_res = SDL_ShowMessageBox(&data, &res_id);
		if (sdl_res != 0)
		{
			res = static_cast<Button>(res_id);
		}
#elif MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_STANDALONE
		NOT_YET_IMPLEMENTED;
#endif
		if (lock_mutex)
		{
			g_common_mutex.unlock();
		}
		return res;
	}
}