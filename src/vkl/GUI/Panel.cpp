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

	void Panel::declarePanelControlMenu(Context& ctx)
	{
		if (ImGui::MenuItem("Auto Resize"))
		{
			_should_auto_resize_x |= true;
			_should_auto_resize_y |= true;
		}
		// Would be nice to be on the same line
		if (ImGui::BeginMenu("Auto Resize..."))
		{
			if (ImGui::MenuItem("Width"))
			{
				_should_auto_resize_x |= true;
			}
			if (ImGui::MenuItem("Height"))
			{
				_should_auto_resize_y |= true;
			}
			ImGui::EndMenu();
		}
		if(ImGui::MenuItem("Collapse", nullptr, nullptr, _is_visible))
		{
			_should_toggle_collapse |= true;
			_should_collapse = true;
		}
		if (ImGui::MenuItem("Roll out", nullptr, nullptr, !_is_visible))
		{
			_should_toggle_collapse |= true;
			_should_collapse = false;
		}
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
		ImVec2 next_size = _window_size;
		ImGuiCond next_size_cond = ImGuiCond_FirstUseEver;
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
		if (_should_auto_resize_x || _should_auto_resize_y)
		{
			next_size_cond = ImGuiCond_Always;
			if (_should_auto_resize_x)
			{
				next_size.x = 0;
			}
			if (_should_auto_resize_y)
			{
				next_size.y = 0;
			}
			_should_auto_resize_x = false;
			_should_auto_resize_y = false;
		}
		if (_should_toggle_collapse)
		{
			ImGui::SetNextWindowCollapsed(_should_collapse, ImGuiCond_Always);
			_should_toggle_collapse = false;
		}
		ImGui::SetNextWindowSize(next_size, next_size_cond);
		_is_visible = ImGui::Begin(_name.c_str(), p_open, flags);
		_open = open;
		_is_hovered = ImGui::IsWindowHovered();
		_has_focus = ImGui::IsWindowFocused();
		if (_is_visible)
		{
			_window_size = ImGui::GetWindowSize();
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
					if (ImGui::BeginMenu("Window"))
					{
						declarePanelControlMenu(ctx);
						ImGui::EndMenu();
					}
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
			_window_size = ImGui::GetWindowSize();
		}
		if (ImGui::BeginPopupContextWindow("RMenu"))
		{
			declarePanelControlMenu(ctx);
			ImGui::EndPopup();
		}
		if (!keep_open)
		{
			ImGui::End();
		}
	}
}