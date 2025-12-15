#pragma once

#include <vkl/App/VkApplication.hpp>

#include <imgui/imgui.h>

namespace vkl::GUI
{
	class Panel : public VkObject
	{
	public:
		using Id = uintptr_t;
		using DeclareFunction = std::function<void(Context&, Panel*)>;

	protected:

		bool _can_close = true;
		bool _open = true;
		bool _is_visible = false;
		bool _is_hovered = false;
		bool _has_focus = false;

		ImGuiWindowFlags _window_flags = ImGuiWindowFlags_None;

		Panel(VkApplication * app, std::string const& name);

	public:

		virtual ~Panel() override;

		virtual void declareMenu(Context& ctx);

		virtual void declareInline(Context& ctx);

		virtual void declare(Context& ctx, bool keep_open=false);

		bool isOpen() const
		{
			return _open;
		}

		void setOpen(bool value = true)
		{
			_open = value;
		}

		bool isVisible() const
		{
			return _is_visible;
		}

		bool isHovered() const
		{
			return _is_hovered;
		}

		bool hasFocus() const
		{
			return _has_focus;
		}

		ImGuiWindowFlags& windowFlags()
		{
			return _window_flags;
		}

		ImGuiWindowFlags windowFlags() const
		{
			return _window_flags;
		}
	};
}