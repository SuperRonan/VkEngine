#define IMGUI_DEFINE_MATH_OPERATORS 1

#include <vkl/GUI/FancyButtons.hpp>
#include <imgui/imgui_internal.h>
#include <utility>
#include <cmath>

namespace ImGui
{
	bool IconButtonEx(const char* str_id, ImVec2 size, ImGuiButtonFlags flags, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;
		if (g.NextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth)
		{
			size.x = g.NextItemData.Width;
		}
		const ImGuiID id = window->GetID(str_id);
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
		const float default_size = GetFrameHeight();
		ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : -1.0f);
		if (!ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

		// Render
		const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		const ImU32 text_col = GetColorU32(ImGuiCol_Text);
		RenderNavCursor(bb, id);
		RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);

		if (render_frame_fn)
		{
			render_frame_fn(render_frame_data, window->DrawList, bb, g.FontSize, text_col);
		}

		IMGUI_TEST_ENGINE_ITEM_INFO(id, str_id, g.LastItemData.StatusFlags);
		return pressed;
	}

	void DrawRectNoCorners(ImDrawList* draw_list, ImRect rect, ImU32 color, float thickness)
	{
		// top
		draw_list->AddLine(ImVec2(rect.GetTL().x + 1, rect.GetTL().y), ImVec2(rect.GetBR().x, rect.GetTL().y), color, thickness);
		// bottom
		draw_list->AddLine(ImVec2(rect.GetTL().x + 1, rect.GetBR().y), ImVec2(rect.GetBR().x, rect.GetBR().y), color, thickness);
		// left
		draw_list->AddLine(ImVec2(rect.GetTL().x, rect.GetTL().y + 1), ImVec2(rect.GetTL().x, rect.GetBR().y), color, thickness);
		// right
		draw_list->AddLine(ImVec2(rect.GetBR().x, rect.GetTL().y + 1), ImVec2(rect.GetBR().x, rect.GetBR().y), color, thickness);
	}

	void RenderDetachIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color)
	{
		float rel_size = 0.75;
		float h = std::max(std::min(rect.GetSize().x, rect.GetSize().y) * rel_size, 1.0f);
		float d = std::max(ImFloor(h / 16.0f), 1.0f);
		float thickness = 1;
		ImVec2 pos = rect.GetTL() + rect.GetSize() * ((1 - rel_size) * 0.5f);
		// Ensure pixel alignment
		pos = ImFloor(pos);
		ImVec2 size = ImFloor(ImVec2(h, h));
		ImVec2 detachment = ImVec2(d, d);
		DrawRectNoCorners(draw_list, ImRect(pos - (detachment + ImVec2(1, 1)), pos + size - (detachment + ImVec2(1, 1))), color, thickness);
		DrawRectNoCorners(draw_list, ImRect(pos + detachment, pos + size + detachment), color, thickness);
	}

	void RenderPlusIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color)
	{
		float size = ImMin(rect.GetWidth(), rect.GetHeight());
		bool odd = (int(size) % 2) != 0;
		float thickness = odd ? 1 : 2;
		float padding_f = 0.15;
		float padding = ImFloor(padding_f * size);
		ImRect r = rect;
		r.Min += ImVec2(padding, padding);
		r.Max -= ImVec2(padding, padding);
		const ImVec2 c = ImFloor(r.GetCenter());
		bool fit_to_rect = reinterpret_cast<uintptr_t>(p_data) & uintptr_t(0x1);
		if (!fit_to_rect)
		{
			float w = r.GetWidth();
			float h = r.GetHeight();
			if (w > h)
			{
				r.Min.x = c.x - h * 0.5f;
				r.Max.x = c.x + h * 0.5f;
			}
			else if (h > w)
			{
				r.Min.y = c.y - w * 0.5f;
				r.Max.y = c.y + w * 0.5f;
			}
		}
		draw_list->AddLine(ImVec2(c.x, r.Min.y), ImVec2(c.x, r.Max.y), color, thickness);
		draw_list->AddLine(ImVec2(r.Min.x, c.y), ImVec2(r.Max.x, c.y), color, thickness);
	}

	void RenderXCrossIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color)
	{
		float d = std::min(rect.GetWidth(), rect.GetHeight());
		float rel_size = std::round(d * 0.5f * 0.7071f - 1.0f);
		float ex = rel_size;
		float thickness = 1;
		draw_list->AddLine((rect.GetCenter() - ImVec2(+ex, +ex)), (rect.GetCenter() + ImVec2(+ex, +ex)), color, thickness);
		draw_list->AddLine((rect.GetCenter() - ImVec2(-ex, +ex)), (rect.GetCenter() + ImVec2(-ex, +ex)), color, thickness);
	}

	void RenderEyeIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color)
	{
		const float r = std::min(rect.GetWidth(), rect.GetHeight());
		draw_list->AddCircleFilled(rect.GetCenter(), ImMax<float>(0.1 * r, 1), color);
		if (true)
		{
			float w = 0.45 * r;
			float h = 0.5 * r;
			draw_list->AddBezierQuadratic(rect.GetCenter() - ImVec2(w, 0), rect.GetCenter() + ImVec2(0, h), rect.GetCenter() + ImVec2(w, 0), color, 1);
			draw_list->AddBezierQuadratic(rect.GetCenter() - ImVec2(w, 0), rect.GetCenter() - ImVec2(0, h), rect.GetCenter() + ImVec2(w, 0), color, 1);
		}
		else
		{
			draw_list->AddCircle(rect.GetCenter(), 0.45 * r, color);
		}
	}

	void RenderBarredIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color)
	{
		uintptr_t options = reinterpret_cast<uintptr_t>(p_data);
		float scale = 0.4;
		uint16_t scale_bits = uint16_t((options >> 2) & 0xffff);
		if (scale_bits != 0)
		{
			scale = float(scale_bits) / float(0xffff);
		}
		if (options == 0 || (options & 0x1) != 0)
		{
			ImVec2 dir = rect.GetTR() - rect.GetBL();
			draw_list->AddLine((rect.GetCenter() + dir * scale), (rect.GetCenter() - dir * scale), color);
		}
		if (options & (0x2))
		{
			ImVec2 dir = rect.GetTL() - rect.GetBR();
			draw_list->AddLine((rect.GetCenter() + dir * scale), (rect.GetCenter() - dir * scale), color);
		}
	}

	void RenderBarredEyeIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color)
	{
		RenderEyeIcon(p_data, draw_list, rect, font_size, color);
		RenderBarredIcon(reinterpret_cast<const void*>(uintptr_t(0x1)), draw_list, rect, font_size, color);
	}

	void RenderTrashCanIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color)
	{
		// Good enough for now
		bool cheat_rounded = true;
		bool odd_width = (int(rect.GetWidth()) % 2) != 0;
		float thickness = 1;
		float rounding = 0;
		float padding_f = 0.15;
		float bottom_padding_f = padding_f;
		ImVec2 padding = ImFloor(rect.GetSize() * padding_f);
		// Relative to the rect
		float can_lid_y = ImFloor(rect.GetHeight() * 0.25f);
		float can_top_y = can_lid_y + 2 + ImFloor(rect.GetHeight() * 1.0f / 13.0f);
		float can_bottom_padding_x = ImFloor(rect.GetWidth() * bottom_padding_f);
		float can_bottom_y = ImCeil((1 - padding_f) * rect.GetHeight());

		// Draw can
		ImVec2 tl = rect.GetTL() + ImVec2(+padding.x, can_top_y);
		ImVec2 bl = rect.GetTL() + ImVec2(+padding.x, can_bottom_y);
		ImVec2 br = rect.GetTR() + ImVec2(-padding.x, can_bottom_y);
		ImVec2 tr = rect.GetTR() + ImVec2(-padding.x, can_top_y);
		if (cheat_rounded)
		{
			draw_list->AddLine(tl + ImVec2(-1, -1), bl + ImVec2(-1, 0), color, thickness);
			draw_list->AddLine(bl, br, color, thickness);
			draw_list->AddLine(br, tr + ImVec2(0, -1), color, thickness);
		}
		else
		{
			draw_list->PathLineTo(tl + ImVec2(-0.5f, -0.5f));
			draw_list->PathLineTo(bl + ImVec2(-0.5f, 0.5f));
			draw_list->PathLineTo(br + ImVec2(0.5f, 0.5f));
			draw_list->PathLineTo(tr + ImVec2(0.5f, -0.5f));
			draw_list->PathStroke(color, ImDrawFlags_None, thickness);
		}

		// Stripes
		float stripes_width = (rect.GetWidth() - 2 * padding.x);
		int n = int(stripes_width / (thickness + padding.x));
		const float base = padding.x + thickness;
		for (int i = 0; i < n; ++i)
		{
			float offset = base;
			float x = offset + i * (thickness + padding.x);
			draw_list->AddLine(rect.GetTL() + ImVec2(x, can_top_y), rect.GetTL() + ImVec2(x, can_bottom_y - thickness), color, thickness);
		}

		// Lid
		draw_list->AddLine(rect.GetTL() + ImVec2(padding.x - thickness, can_lid_y), rect.GetTR() + ImVec2(-padding.x + thickness, can_lid_y), color, thickness);
		// Handle
		ImVec2 handle_size_f = ImVec2(0.25, 0.15);
		ImVec2 handle_size = rect.GetSize() * handle_size_f;
		draw_list->AddBezierQuadratic(
			ImVec2(rect.GetCenter().x - handle_size.x * 0.5, rect.Min.y + can_lid_y),
			ImVec2(rect.GetCenter().x, rect.Min.y + can_lid_y - handle_size.y),
			ImVec2(rect.GetCenter().x + handle_size.x * 0.5, rect.Min.y + can_lid_y),
			color, thickness);
	}

	bool DetachButton()
	{
		bool res = IconButton("Detach", RenderDetachIcon);
		//bool res = ImGui::ArrowButton("Detach", ImGuiDir_Up);
		ImGui::SetItemTooltip("Detach panel");
		return res;
	}

	bool XCrossButton(const char* tooltip, bool small_button)
	{
		bool res = IconButton("XCross", RenderXCrossIcon, nullptr, small_button);
		if (tooltip)
		{
			ImGui::SetItemTooltip(tooltip);
		}
		return res;
	}

	bool TrashButton(const char* tooltip, bool small_button)
	{
		bool res = IconButton("Trash", RenderTrashCanIcon, nullptr, small_button);
		if (tooltip)
		{
			ImGui::SetItemTooltip(tooltip);
		}
		return res;
	}

	bool PlusButton(const char* tooltip, bool small_button)
	{
		bool res = IconButton("Plus", RenderPlusIcon, nullptr, small_button);
		if (tooltip)
		{
			ImGui::SetItemTooltip(tooltip);
		}
		return res;
	}




	bool InboxCheckboxEx(const char* label, bool* v, ImVec2 box_size, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		const ImVec2 pos = window->DC.CursorPos;
		const ImRect total_bb(pos, pos + box_size);
		ItemSize(total_bb, style.FramePadding.y);
		const bool is_visible = ItemAdd(total_bb, id);
		const bool is_multi_select = (g.LastItemData.ItemFlags & ImGuiItemFlags_IsMultiSelect) != 0;
		if (!is_visible)
			if (!is_multi_select || !g.BoxSelectState.UnclipMode || !g.BoxSelectState.UnclipRect.Overlaps(total_bb)) // Extra layer of "no logic clip" for box-select support
			{
				IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
				return false;
			}

		// Range-Selection/Multi-selection support (header)
		bool checked = *v;
		if (is_multi_select)
			MultiSelectItemHeader(id, &checked, NULL);

		bool hovered, held;
		bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);

		// Range-Selection/Multi-selection support (footer)
		if (is_multi_select)
			MultiSelectItemFooter(id, &checked, &pressed);
		else if (pressed)
			checked = !checked;

		if (*v != checked)
		{
			*v = checked;
			pressed = true; // return value
			MarkItemEdited(id);
		}

		const ImRect check_bb = total_bb;
		const bool mixed_value = (g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0;
		if (is_visible)
		{
			RenderNavCursor(total_bb, id);
			RenderFrame(check_bb.Min, check_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
			ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
			ImU32 text_col = GetColorU32(ImGuiCol_Text);
			ImU32 disabled_col = GetColorU32(ImGuiCol_TextDisabled);
			if (render_frame_fn)
			{
				render_frame_fn(render_frame_data, window->DrawList, total_bb, ImGui::GetFontSize(), text_col);
			}
			if (mixed_value)
			{
				// Undocumented tristate/mixed/indeterminate checkbox (#2644)
				// This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
				// 
				// TODO
				// 
				//ImVec2 pad(ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)), ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)));
				//window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
				IM_ASSERT("Not Yet Implemented");
			}
			else if (*v)
			{
				float rounding = GetStyle().FrameRounding;
				window->DrawList->AddRect(total_bb.GetTL(), total_bb.GetBR(), check_col, rounding, 0, GetStyle().FrameBorderSize);
				//DrawRectNoCorners(window->DrawList, ImRect(total_bb.GetTL(), total_bb.GetBR() - ImVec2(1, 1)), check_col);
			}
		}
		const ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
		if (g.LogEnabled)
			LogRenderedText(&label_pos, mixed_value ? "[~]" : *v ? "[x]" : "[ ]");

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return pressed;
	}

	bool InboxCheckbox(const char* label, bool* v, ImVec2 box_size)
	{
		auto& style = ImGui::GetStyle();
		const float padding = style.FramePadding.x;
		ImVec2 text_size = CalcTextSize(label);
		ImVec2 sz = box_size;
		if (box_size.y == 0.0f)
		{
			sz.y = ImGui::GetFrameHeight();
		}
		if (box_size.x == 0.0f)
		{
			sz.x = ImMax(text_size.x + 2 * padding, sz.y);
		}
		struct RenderData
		{
			const char* label;
			ImFont* font;
			ImVec2 size;
			ImVec2 padding;
		};
		RenderData render_data{
			.label = label,
			.font = ImGui::GetFont(),
			.size = text_size,
			.padding = style.FramePadding,
		};
		auto render_label = [](const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color)
			{
				const RenderData* render_data = reinterpret_cast<const RenderData*>(p_data);
				ImVec2 pos = rect.GetTL();
				// Center text in rect
				pos = pos + (rect.GetSize() - render_data->size) * 0.5;
				pos = ImFloor(pos);
				pos = ImMax(pos, rect.GetTL());
				draw_list->AddText(render_data->font, font_size, pos, color, render_data->label, nullptr, render_data->size.x);
			};
		return InboxCheckboxEx(label, v, sz, render_label, &render_data);
	}

	bool InboxCheckbox(const char* label, bool* v, bool small_box)
	{
		ImVec2 size = ImVec2(0, 0);
		if (small_box)
		{
			size.y = ImGui::GetFontSize();
		}
		return InboxCheckbox(label, v, size);
	}

	bool IconCheckbox(const char* label, bool* v, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data, ImVec2 box_size)
	{
		return InboxCheckboxEx(label, v, box_size, render_frame_fn, render_frame_data);
	}

	ImVec2 GetDefaultBoxSize(bool small_box)
	{
		float sz = 0;
		if (small_box)
		{
			sz = ImGui::GetFontSize();
		}
		else
		{
			sz = ImGui::GetFrameHeight();
		}
		ImVec2 size = ImVec2(sz, sz);
		return size;
	}

	bool IconCheckbox(const char* label, bool* v, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data, bool small_box)
	{
		return IconCheckbox(label, v, render_frame_fn, render_frame_data, GetDefaultBoxSize(small_box));
	}

	bool FlipIconButton(const char* label, bool* v, ImVec2 box_size,
		ButtonIconDrawFunction false_render_frame_fn, const void* false_render_frame_data,
		ButtonIconDrawFunction true_render_frame_fn, const void* true_render_frame_data
	)
	{
		if (ImGui::IconButtonEx(label, box_size, ImGuiButtonFlags_None, *v ? true_render_frame_fn : false_render_frame_fn, *v ? true_render_frame_data : false_render_frame_data))
		{
			*v = !*v;
			return true;
		}
		return false;
	}

	bool FlipIconButton(const char* label, bool* v,
		ButtonIconDrawFunction false_render_frame_fn, const void* false_render_frame_data,
		ButtonIconDrawFunction true_render_frame_fn, const void* true_render_frame_data,
		bool small_box
	)
	{
		return FlipIconButton(label, v, GetDefaultBoxSize(small_box), false_render_frame_fn, false_render_frame_data, true_render_frame_fn, true_render_frame_data);
	}

	bool BarredIconButton(const char* label, bool* v, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data, bool small_box, bool x_cross)
	{
		struct RenderData
		{
			ButtonIconDrawFunction render_fn;
			const void* render_data;
			bool x_cross;
		};
		RenderData render_data{
			.render_fn = render_frame_fn,
			.render_data = render_frame_data,
			.x_cross = x_cross,
		};
		ButtonIconDrawFunction render_barred_icon = [](const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color)
			{
				const RenderData* render_data = reinterpret_cast<const RenderData*>(p_data);
				if (render_data->render_fn)
				{
					render_data->render_fn(render_data->render_data, draw_list, rect, font_size, color);
				}
				uintptr_t bar_options = render_data->x_cross ? 0b11 : 0b1;
				float scale = 0.35;
				bar_options |= (uintptr_t(scale * 0xffff) << 2);
				RenderBarredIcon(reinterpret_cast<const void*>(bar_options), draw_list, rect, font_size, color);
			};
		return FlipIconButton(label, v, render_barred_icon, &render_data, render_frame_fn, render_frame_data, small_box);
	}

	bool ArrowFlipButton(const char* label_id)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImGuiID storage_id = window->GetID(label_id);
		ImGuiStorage* storage = window->DC.StateStorage;
		bool is_open = storage->GetBool(storage_id);
		ImGuiDir button_dir = is_open ? ImGuiDir_Up : ImGuiDir_Down;
		if (ImGui::ArrowButton(label_id, button_dir))
		{
			is_open = !is_open;
			storage->SetBool(storage_id, is_open);
		}
		return is_open;
	}

}