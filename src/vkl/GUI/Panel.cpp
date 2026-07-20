#include <vkl/GUI/Panel.hpp>
#include <vkl/GUI/Context.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace ImGui
{
	ImGuiID GetOrCreateWindowDockID(ImGuiWindow* window)
	{
		ImGuiID res = window->DockId;
		if (res == 0)
		{
			res = DockBuilderAddNode();
			DockBuilderSetNodePos(res, window->Pos);
			DockBuilderSetNodeSize(res, window->Size);
			DockBuilderDockWindow(window->Name, res);
			DockBuilderFinish(res);
		}
		return res;
	}
}

namespace vkl::GUI
{
	Panel::Panel(VkApplication* app, std::string_view name):
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

	ImGuiID Panel::getOrCreateDockId(Context& ctx)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (!window || std::string_view(window->Name) != _name)
		{
			window = ImGui::FindWindowByName(_name.c_str());
		}
		if (window)
		{
			_dock_id = ImGui::GetOrCreateWindowDockID(window);
		}
		else
		{
			_dock_id = 0;
		}
		return _dock_id;
	}

	void Panel::setDockID(ImGuiID id, bool set_always)
	{
		_set_dock_id = true;
		_dock_id = id;
		_set_dock_id_always = set_always;
	}

	void Panel::declare(Context& ctx, bool keep_open)
	{
		ImGuiWindowFlags flags = _window_flags;
		bool open = _open;
		bool* p_open = _can_close ? &open : nullptr;
		// Not perfect auto size, will do for now
		ImGui::SetNextWindowSize(_window_initial_size, ImGuiCond_FirstUseEver);
		// TODO find a nice initial pos from context with ImGui::SetNextWindowPos
		if (_set_dock_id)
		{
			ImGui::SetNextWindowDockID(_dock_id, _set_dock_id_always ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
			_set_dock_id = false;
		}
		if (_next_take_focus)
		{
			ImGui::SetNextWindowFocus();
			_next_take_focus = false;
		}
		if (_next_no_focus_on_appearing)
		{
			flags |= ImGuiWindowFlags_NoFocusOnAppearing;
			_next_no_focus_on_appearing = false;
		}
		if (_next_no_bring_to_front_on_focus)
		{
			flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
			_next_no_bring_to_front_on_focus = false;
		}
		_is_visible = ImGui::Begin(_name.c_str(), p_open, flags);
		_open = open;
		_is_hovered = ImGui::IsWindowHovered();
		_has_focus = ImGui::IsWindowFocused();
		if (_is_visible)
		{
			_dock_id = ImGui::GetWindowDockID();
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
			if (_can_close && _has_focus)
			{
				if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_W, ImGuiInputFlags_None, ImGui::GetCurrentWindowRead()->ID))
				{
					setOpen(false);
				}
			}
		}
		auto window = ImGui::GetCurrentWindowRead();
		//ImGui::SetNextWindowContentSize(_window_cotent_size);
		if (!keep_open)
		{
			ImGui::End();
		}
	}
}