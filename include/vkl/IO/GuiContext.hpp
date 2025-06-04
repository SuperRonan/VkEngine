#pragma once

#include <imgui/imgui.h>

#include <memory>
#include "FileDialog.hpp"

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

		std::shared_ptr<FileDialog> _common_file_dialog;

	public:

		struct CreateInfo
		{
			ImGuiContext * imgui_context = nullptr;
			std::shared_ptr<GUIStyle> style = nullptr;
			std::shared_ptr<FileDialog> common_file_dialog = nullptr;
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
		
		SDL_Window* getCurrentWindow()
		{
			return static_cast<SDL_Window*>(ImGui::GetWindowViewport()->PlatformHandleRaw);
		}

		const std::shared_ptr<FileDialog>& getCommonFileDialog()
		{
			return _common_file_dialog;
		}
	};
}