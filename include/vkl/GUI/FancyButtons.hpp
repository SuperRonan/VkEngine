#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace ImGui
{
	extern ImVec2 GetDefaultBoxSize(bool small_box);

	using ButtonIconDrawFunction = void(*)(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern bool IconButtonEx(const char* str_id, ImVec2 size, ImGuiButtonFlags flags, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data = nullptr);

	static inline bool IconButton(const char* str_id, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data = nullptr, bool small_button = false)
	{
		float sz = small_button ? ImGui::GetTextLineHeight() : ImGui::GetFrameHeight();
		return IconButtonEx(str_id, ImVec2(sz, sz), ImGuiButtonFlags_None, render_frame_fn, render_frame_data);
	}

	static inline bool SquareButton(const char* label, bool small_button = false)
	{
		float sz = small_button ? ImGui::GetTextLineHeight() : ImGui::GetFrameHeight();
		return Button(label, ImVec2(sz, sz));
	}

	extern void DrawRectNoCorners(ImDrawList* draw_list, ImRect rect, ImU32 color, float thickness = 1);

	extern void RenderDetachIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderPlusIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderXCrossIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderEyeIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderBarredIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderBarredEyeIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderTrashCanIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern bool DetachButton();

	extern bool XCrossButton(const char* tooltip = nullptr, bool small_button = false);

	extern bool TrashButton(const char* tooltip = nullptr, bool small_button = false);

	extern bool PlusButton(const char* tooltip = nullptr, bool small_button = false);




	extern bool InboxCheckboxEx(const char* label, bool* v, ImVec2 box_size, ButtonIconDrawFunction render_frame_fn = nullptr, const void* render_frame_data = nullptr);

	// Explicit sizing of the box
	extern bool InboxCheckbox(const char* label, bool* v, ImVec2 box_size);

	// Implicit sizing of the box
	extern bool InboxCheckbox(const char* label, bool* v, bool small_box = false);

	// Explicit sizing of the box
	extern bool IconCheckbox(const char* label, bool* v, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data, ImVec2 box_size);

	// Implicit sizing of the box
	extern bool IconCheckbox(const char* label, bool* v, ButtonIconDrawFunction render_frame_fn = nullptr, const void* render_frame_data = nullptr, bool small_box = false);

	extern bool FlipIconButton(const char* label, bool* v, ImVec2 box_size,
		ButtonIconDrawFunction false_render_frame_fn = nullptr, const void* false_render_frame_data = nullptr,
		ButtonIconDrawFunction true_render_frame_fn = nullptr, const void* true_render_frame_data = nullptr
	);

	extern bool FlipIconButton(const char* label, bool* v,
		ButtonIconDrawFunction false_render_frame_fn = nullptr, const void* false_render_frame_data = nullptr,
		ButtonIconDrawFunction true_render_frame_fn = nullptr, const void* true_render_frame_data = nullptr,
		bool small_button = false
	);

	static inline bool FlipIconButton(const char* label, bool* v,
		ButtonIconDrawFunction false_render_frame_fn = nullptr,
		ButtonIconDrawFunction true_render_frame_fn = nullptr,
		bool small_button = false
	) {
		return FlipIconButton(label, v, false_render_frame_fn, nullptr, true_render_frame_fn, nullptr, small_button);
	}

	extern bool BarredIconButton(const char* label, bool* v, ButtonIconDrawFunction render_frame_fn = nullptr, const void* render_frame_data = nullptr, bool small_box = false, bool x_cross = false);
}