#pragma once

#include <Core/VulkanCommons.hpp>

#if _WINDOWS
#include <Windows.h>
#endif

#define MESSAGE_POPUP_USE_STANDALONE 0
#define MESSAGE_POPUP_USE_WINDOWS 1

#if _WINDOWS
#define MESSAGE_POPUP_POLICY MESSAGE_POPUP_USE_WINDOWS
#endif

#ifndef MESSAGE_POPUP_POLICY
#define MESSAGE_POPUP_POLICY MESSAGE_POPUP_USE_STANDALONE 
#endif

#include <string>

namespace vkl
{
	class MessagePopUp
	{
	public:
		
		enum class Type
		{
#if MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_STANDALONE
			Error,
			Stop = Error,
			Hand = Error,
			
			Question,
			
			Warning,
			Exclamation = Warning,
			
			Information,
			Asterisk = Information,
#elif MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_WINDOWS
			Error = MB_ICONERROR,
			Stop = MB_ICONSTOP,
			Hand = MB_ICONHAND,

			Question = MB_ICONQUESTION,

			Warning = MB_ICONWARNING,
			Exclamation = MB_ICONEXCLAMATION,

			Information = MB_ICONINFORMATION,
			Asterisk = MB_ICONASTERISK,
#endif
		};

		enum class Button
		{
#if MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_STANDALONE
			Ok,
			Cancel,
			Abort,
			Retry,
			Ignore,
			Yes,
			No,
			TryAgain,
			Continue,
#elif MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_WINDOWS
			Ok = IDOK,
			Cancel = IDCANCEL,
			Abort = IDABORT,
			Retry = IDRETRY,
			Ignore = IDIGNORE,
			Yes = IDYES,
			No = IDNO,
			TryAgain = IDTRYAGAIN,
			Continue = IDCONTINUE,
#endif
		};
		
	protected:
		
		Type _type;
		std::string _title = {};
		std::string _message = {};
		std::vector<Button> _buttons = {};
		bool _beep = false;
		bool _log_cout = false;

#if MESSAGE_POPUP_POLICY == MESSAGE_POPUP_USE_WINDOWS
		UINT getButtonsFlagsWindows() const;
#endif

	public:

		struct CreateInfo
		{
			Type type;
			std::string title = {};
			std::string message = {};
			std::vector<Button> buttons = {};
			bool beep = true;
			bool log_cout = true;
		};
		using CI = CreateInfo;

		MessagePopUp(CreateInfo const& ci);
	};

	class SynchMessagePopUp : public MessagePopUp
	{
	protected:

	public:

		struct CreateInfo
		{
			Type type;
			std::string title = {};
			std::string message = {};
			std::vector<Button> buttons = {};
			bool beep = true;
			bool log_cout = true;
		};
		using CI = CreateInfo;

		SynchMessagePopUp(CreateInfo const& ci) :
			MessagePopUp(MessagePopUp::CI{
				.type = ci.type,
				.title = ci.title,
				.message = ci.message,
				.buttons = ci.buttons,
				.beep = ci.beep,
				.log_cout = ci.log_cout,
			})
		{

		}

		Button operator()()const;
	};
}