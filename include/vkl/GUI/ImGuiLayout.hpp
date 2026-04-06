#pragma once

#include <imgui/imgui.h>

namespace ImGui::Layout
{
	extern ImVec2 Button(const char* label, bool small_button = false);

	extern float SameLine();
}