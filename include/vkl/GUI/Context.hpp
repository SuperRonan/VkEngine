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

		std::vector<Color> stack_colors;
	};

	enum class EnumStyle
	{
		Label = 0,
		Decimal = 1,
		Hexa = 2,
		MAX_VALUE = Hexa,
		Default = Label,
	};

	static inline const char* GetEnumStyleFormat(EnumStyle style)
	{
		const char* fmt = nullptr;
		if (style == EnumStyle::Decimal)
		{
			fmt = "%d";
		}
		else if (style == EnumStyle::Hexa)
		{
			fmt = "0x%x";
		}
		return fmt;
	}

	extern EnumStyle CycleNextEnumStyle(EnumStyle es);

	// p_style may be nullptr
	extern bool DeclareEnumStyleButtonSwitch(EnumStyle* p_style);
	struct TransientPayload
	{
		std::shared_ptr<VkObject> object;
	};

	class Panel;
	class PanelHolder;
	
	class Context
	{
	protected:

		ImGuiContext * _imgui_context;

		std::shared_ptr<Style> _style;

		uint _stack_counter = 0;
		bool _keep_drag_drop_payload = false;

		std::shared_ptr<FileDialog> _common_file_dialog;

		MyVector<PanelHolder*> _panel_holder_stack;
		MyVector<Panel*> _panel_stack;

		TransientPayload _drag_drop_payload;
		TransientPayload _clipboard_payload;


		EnumStyle _enum_style = EnumStyle::Default;
	public:

		struct CreateInfo
		{
			ImGuiContext * imgui_context = nullptr;
			std::shared_ptr<Style> style = nullptr;
			std::shared_ptr<FileDialog> common_file_dialog = nullptr;
		};
		using CI = CreateInfo;

		Context(CreateInfo const& ci);

		void begin();

		void end();

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

		void pushPanel(Panel* panel);

		void popPanel();

		PanelHolder* getTopPanelHolder(uint index = 0) const;

		PanelHolder* getBottomPanelHolder(uint index = 0) const;

		std::span<PanelHolder* const> getPanelHolderStack() const;

		Panel* getTopPanel(uint index = 0) const;

		Panel* getBottomPanel(uint index = 0) const;

		std::span<Panel* const> getPanelStack() const;

		Style::Color pushStack();

		Style::Color popStack();

		TransientPayload& getDragDropPayload()
		{
			return _drag_drop_payload;
		}

		TransientPayload& getClipboardPayload()
		{
			return _clipboard_payload;
		}

		void keepDragDropPayload()
		{
			_keep_drag_drop_payload = true;
		}

		void clearTemporaryData();

		EnumStyle* pEnumStyle()
		{
			return &_enum_style;
		}
	};
}