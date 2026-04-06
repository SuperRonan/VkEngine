#include <vkl/GUI/ImGuiLayout.hpp>

#include <imgui/imgui_internal.h>

namespace ImGui::Layout
{
	ImVec2 Button(const char* label, bool small_button)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();
		ImVec2 res = ImVec2(0.0f, 0.0f);
		res = ImGui::CalcTextSize(label);
		if (!small_button)
		{
			res.y = ImGui::GetFrameHeight();
			res.x += g.Style.FramePadding.x * 2.0f;
		}
		else
		{
			
		}
		return res;
	}

	float SameLine()
	{
		ImGuiContext& g = *GImGui;
		return g.Style.ItemSpacing.x;
	}
}