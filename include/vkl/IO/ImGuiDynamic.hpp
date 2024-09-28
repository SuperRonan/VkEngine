#pragma once

#include <vkl/Core/DynamicValue.hpp>

#include <imgui/imgui.h>

namespace vkl
{
	
	template <class T, std::convertible_to<std::function<bool(const char*, T&)>> GuiFunc>
	static bool GUIDeclareDynamic(const char* label, Dyn<T>& value, GuiFunc const& imgui_f)
	{
		bool res = false;
		if (value.hasValue())
		{
			value.value();
			T & v = value.getCachedValueRef();
			bool read_only = !value.canSetValue();
			ImGui::BeginDisabled(read_only);
			res = imgui_f(label, v);
			ImGui::EndDisabled();
		}
		else
		{
			ImGui::Text("%s: No value", label);
		}
		return res;
	}
}