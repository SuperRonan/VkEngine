#include <vkl/GUI/Panel.hpp>
#include <vkl/GUI/Context.hpp>

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
		// Not perfect auto size, will do for now
		ImGui::SetNextWindowSize(_window_initial_size, ImGuiCond_FirstUseEver);
		// TODO find a nice initial pos from context with ImGui::SetNextWindowPos
		_is_visible = ImGui::Begin(_name.c_str(), p_open, flags);
		_is_hovered = ImGui::IsWindowHovered();
		_has_focus = ImGui::IsWindowFocused();
		if (_is_visible)
		{
			if (!_disable_from_ctx_stack)
			{
				ctx.pushPanel(this);
			}
			if (flags & ImGuiWindowFlags_MenuBar)
			{
				if (ImGui::BeginMenuBar())
				{
					declareMenu(ctx);
					ImGui::EndMenuBar();
				}
			}
			declareInline(ctx);
			if (!_disable_from_ctx_stack)
			{
				ctx.popPanel();
			}
		}
		if (!keep_open)
		{
			ImGui::End();
		}
	}
}