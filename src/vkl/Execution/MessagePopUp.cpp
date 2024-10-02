#include <vkl/Execution/MessagePopUp.hpp>

#include <algorithm>
#include <iostream>

namespace vkl
{
	MessagePopUp::MessagePopUp(CreateInfo const& ci):
		_type(ci.type),
		_title(ci.title),
		_message(ci.message),
		_buttons(ci.buttons),
		_beep(ci.beep),
		_logger(ci.logger)
	{}

#if MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_WINDOWS
	UINT MessagePopUp::getButtonsFlagsWindows() const
	{
		UINT res = 0;
		std::vector<Button> sorted_buttons = _buttons;
		std::sort(sorted_buttons.begin(), sorted_buttons.end());

		if (sorted_buttons.size() == 1)
		{
			if(sorted_buttons[0] == Button::Ok)
			{
				res = MB_OK;
			}
		}
		else if(sorted_buttons.size() == 2)
		{
			Button b0 = sorted_buttons[0];
			Button b1 = sorted_buttons[1];
			if (b0 == Button::Ok && b1 == Button::Cancel)
			{
				res = MB_OKCANCEL;
			}
			else if (b0 == Button::Cancel && b1 == Button::Retry)
			{
				res = MB_RETRYCANCEL;
			}
			else if(b0 == Button::Yes && b1 == Button::No)
			{
				res = MB_YESNO;
			}
		}
		else if (sorted_buttons.size() == 3)
		{
			Button b0 = sorted_buttons[0];
			Button b1 = sorted_buttons[1];
			Button b2 = sorted_buttons[2];
			NOT_YET_IMPLEMENTED;
		}
		return res;
	}
#endif

	MessagePopUp::Button SynchMessagePopUp::operator()(bool lock_mutex)const
	{
		if (lock_mutex)
		{
			g_common_mutex.lock();
		}
		if (_beep)
		{
#if MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_WINDOWS
			MessageBeep(static_cast<UINT>(_type));
#endif
		}
		Button res;
#if MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_WINDOWS
		
		if (_logger)
		{
			_logger->log(std::format("{}\n{}", _title, _message));
		}

		UINT flags = static_cast<UINT>(_type) | getButtonsFlagsWindows();
		int ires = MessageBoxA(nullptr, _message.c_str(), _title.c_str(), flags);
		res = static_cast<Button>(ires);
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