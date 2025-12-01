#pragma once

#include <imgui/imgui.h>

#include <memory>
#include "FileDialog.hpp"

namespace vkl::GUI
{
	struct Style
	{
		using Color = ImVec4;

		Color valid_green;
		Color invalid_red;
		Color warning_yellow;
	};

	class Panel;
	class PanelHolder;
	
	class Context
	{
	protected:

		ImGuiContext * _imgui_context;

		std::shared_ptr<Style> _style;

		std::shared_ptr<FileDialog> _common_file_dialog;

		MyVector<PanelHolder*> _panel_holder_stack;

	public:

		struct CreateInfo
		{
			ImGuiContext * imgui_context = nullptr;
			std::shared_ptr<Style> style = nullptr;
			std::shared_ptr<FileDialog> common_file_dialog = nullptr;
		};
		using CI = CreateInfo;

		Context(CreateInfo const& ci);

		ImGuiContext* getImGuiContext() const
		{
			return _imgui_context;
		}

		Style const& style()const
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

		void pushPanelHolder(PanelHolder* panel);

		void popPanelHolder();

		PanelHolder* getTopPanelHolder(uint index = 0);

		PanelHolder* getBottomPanelHolder(uint index = 0);
	};
}