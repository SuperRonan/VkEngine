#pragma once

#include <vkl/GUI/Panel.hpp>

namespace vkl::GUI
{
	struct DetachButtonRes
	{
		bool value = {};
		Panel::FocusAction focus_action = {};

		operator bool() const
		{
			return value;
		}
	};

	extern DetachButtonRes DetachButton2(Context& ctx);

	class ObjectInlineInspector
	{
	public:
		struct AcceptObjectResult
		{
			bool accept = false;
			bool invalid_type = false;
			const char* reason = nullptr;
		};
		using AcceptObjectFnPtr = AcceptObjectResult(*)(const VkObject* current, const VkObject* incoming, const void* data);

	protected:
		std::string _label;
		VkObject* _target = nullptr;
		std::shared_ptr<Panel> _panel = {};
		bool _enable_source = false;
		bool _accept_nullptr = false;
		bool _disable_create = false;
		bool _hide_create_remove_button = false;

		AcceptObjectFnPtr _accept_fn = nullptr;
		const void* _accept_data = nullptr;
	public:

		ObjectInlineInspector(std::string_view label = {});

		ObjectInlineInspector(ObjectInlineInspector const&) = default;
		ObjectInlineInspector(ObjectInlineInspector &&) = default;

		~ObjectInlineInspector() = default;

		void openPanelIFN(Context& ctx, std::shared_ptr<VkObject> const& target, bool set_open, Panel::FocusAction focus_action = Panel::FocusAction::None);

		void setTargetIFN(Context& ctx, std::shared_ptr<VkObject> const& target);

		void setAcceptFunction(AcceptObjectFnPtr fn, const void* data = nullptr);

		void setEnableSource(bool enable = true)
		{
			_enable_source = enable;
		}

		void setAcceptNullptr(bool accept = true)
		{
			_accept_nullptr = accept;
		}

		void setDisableCreation(bool disable = true)
		{
			_disable_create = disable;
		}

		void setHideCreateRemoveButton(bool hide = true)
		{
			_hide_create_remove_button = hide;
		}

		bool declareInline(Context& ctx, std::shared_ptr<VkObject>const& target, std::shared_ptr<VkObject>* dst_target=nullptr);

		void clear();
	};
}