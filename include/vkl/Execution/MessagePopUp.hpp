#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/Core/LogOptions.hpp>

#include <string>

namespace vkl
{
	class VkWindow;

	class MessagePopUp
	{
	public:
		
		enum class Type : uint
		{
			Error = 0,
			Stop = Error + 1,
			Hand = Stop + 1,

			Question = Hand + 1,

			Warning = Question + 1,
			Exclamation = Warning + 1,

			Information = Exclamation + 1,
			Asterisk = Information + 1,

			MAX_VALUE = Asterisk + 1,
		};

		enum class Button : uint
		{
			Ok = 0,
			Cancel = Ok + 1,
			Abort = Cancel + 1,
			Retry = Abort + 1,
			Ignore = Retry + 1,
			Yes = Ignore + 1,
			No = Yes + 1,
			Close = No + 1,
			Help = Close + 1,
			TryAgain = Help + 1,
			Continue = TryAgain + 1,
			
			MAX_VALUE = Continue + 1,
		};
		
	protected:
		
		Type _type;
		std::string _title = {};
		std::string _message = {};
		std::vector<Button> _buttons = {};
		bool _beep = false;
		const Logger * _logger = nullptr;
		VkWindow * _parent_window = nullptr;

	public:

		struct CreateInfo
		{
			Type type;
			std::string title = {};
			std::string message = {};
			std::vector<Button> buttons = {};
			bool beep = true;
			const Logger * logger = nullptr;
			VkWindow * parent_window = nullptr;
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
			const Logger * logger = nullptr;
			VkWindow* parent_window = nullptr;
		};
		using CI = CreateInfo;

		SynchMessagePopUp(CreateInfo const& ci) :
			MessagePopUp(MessagePopUp::CI{
				.type = ci.type,
				.title = ci.title,
				.message = ci.message,
				.buttons = ci.buttons,
				.beep = ci.beep,
				.logger = ci.logger,
				.parent_window = ci.parent_window,
			})
		{

		}

		Button operator()(bool lock_mutex = true)const;
	};
}