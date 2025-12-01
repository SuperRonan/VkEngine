#pragma once

#include <vkl/App/VkApplication.hpp>

namespace vkl::GUI
{
	class Panel : public VkObject
	{
	public:
		using Id = uintptr_t;

	protected:

		bool _can_close = true;
		bool _open = true;
		bool _show_menu = false;

		Panel(VkApplication * app, std::string const& name);

	public:

		virtual ~Panel() override;

		virtual void declareMenu(Context& ctx);

		virtual void declareInline(Context& ctx);

		virtual void declare(Context& ctx);

		bool isOpen() const
		{
			return _open;
		}

		void setOpen(bool value = true)
		{
			_open = value;
		}
	};
}