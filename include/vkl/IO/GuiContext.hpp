#pragma once

#include <imgui/imgui.h>

#include <memory>

namespace vkl
{
	struct GUIStyle
	{
		using Color = ImVec4;

		Color valid_green;
		Color invalid_red;
		Color warning_yellow;
	};
	
	class GuiContext
	{
	protected:

		ImGuiContext * _imgui_context;

		std::shared_ptr<GUIStyle> _style;

	public:

		struct CreateInfo
		{
			ImGuiContext * imgui_context = nullptr;
			std::shared_ptr<GUIStyle> style = nullptr;
		};
		using CI = CreateInfo;

		GuiContext(CreateInfo const& ci);

		ImGuiContext* getImGuiContext() const
		{
			return _imgui_context;
		}

		GUIStyle const& style()const
		{
			assert(_style);
			return *_style;
		}
	};
}