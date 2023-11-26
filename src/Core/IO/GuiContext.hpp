#pragma once

#include <imgui/imgui.h>

namespace vkl
{
	class GuiContext
	{
	protected:

		ImGuiContext * _imgui_context;

	public:

		struct CreateInfo
		{
			ImGuiContext * imgui_context = nullptr;
		};
		using CI = CreateInfo;

		GuiContext(CreateInfo const& ci);

		ImGuiContext* getImGuiContext() const
		{
			return _imgui_context;
		}
	};
}