#include <vkl/GUI/PopUp.hpp>

#include <imgui/imgui_internal.h>

#include <vkl/GUI/ImGuiLayout.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>

namespace vkl::GUI
{
	void PopUp::open(Context& ctx)
	{
		if (!_open)
		{
			ImGui::OpenPopup(_name.c_str(), _flags);
			_open = true;
		}
	}

	void PopUp::close()
	{
		if (_open)
		{
			// TODO
			_open = false;
		}
	}

	PopUp::Result PopUp::declare(Context& ctx)
	{
		Result res = Result::Cancel;
		bool begin = false;
		if (_style == Style::Modal)
		{
			begin = ImGui::BeginPopup(_name.c_str(), _window_flags);
		}
		else // if (_style == Style::Classic)
		{
			bool p_open = true;
			begin = ImGui::BeginPopupModal(_name.c_str(), &p_open, _window_flags);
		}
		if (begin)
		{
			res = declareInline(ctx);
			bool close = false;
			close |= (res == Result::Cancel);
			close |= (res == Result::Create && !_keep_open_on_create);
			if (close)
			{
				ImGui::CloseCurrentPopup();
				_open = false;
			}
			ImGui::EndPopup();
		}
		else
		{
			if (_open)
			{
				_open = false;
			}
		}
		return res;
	}

	PopUp::Result PopUp::DeclareResultButtons(Context& ctx, bool enable_creation, const char* create_label)
	{
		const char* cancel_label = "Cancel";
		if (!create_label)
		{
			create_label = "Create";
		}
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		{
			float width = 0;
			width += ImGui::Layout::Button(create_label).x;
			width += ImGui::Layout::SameLine();
			width += ImGui::Layout::Button(cancel_label).x;
			ImGui::CenterNextItem(width);
		}
		Result res = Result::Pending;
		ImGui::BeginDisabled(!enable_creation);
		
		if (ImGui::Button(create_label))
		{
			res = Result::Create;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button(cancel_label))
		{
			res = Result::Cancel;
		}
		return res;
	}
}