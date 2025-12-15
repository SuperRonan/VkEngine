#include <vkl/GUI/Panel.hpp>

#include <imgui/imgui.h>

namespace vkl::GUI
{
	Panel::Panel(VkApplication* app, std::string const& name):
		VkObject(app, name)
	{

	}

	Panel::~Panel()
	{

	}

	void Panel::declareInline(Context& ctx)
	{

	}

	void Panel::declareMenu(Context& ctx)
	{

	}

	void Panel::declare(Context& ctx, bool keep_open)
	{
		ImGuiWindowFlags flags = _window_flags;
		bool* p_open = _can_close ? &_open : nullptr;
		_is_visible = ImGui::Begin(_name.c_str(), p_open, flags);
		_is_hovered = ImGui::IsWindowHovered();
		_has_focus = ImGui::IsWindowFocused();
		if (_is_visible)
		{
			if (flags & ImGuiWindowFlags_MenuBar)
			{
				if (ImGui::BeginMenuBar())
				{
					declareMenu(ctx);
					ImGui::EndMenuBar();
				}
			}
			declareInline(ctx);
		}
		if (!keep_open)
		{
			ImGui::End();
		}
	}
}