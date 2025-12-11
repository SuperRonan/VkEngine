#define IMGUI_DEFINE_MATH_OPERATORS

#include <vkl/GUI/ImGuiUtils.hpp>
#include <cassert>

#include <vkl/Maths/Transforms.hpp>

#include <numbers>

#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace vkl
{
	ImGuiListSelection::ImGuiListSelection(CreateInfo const& ci) :
		_name(ci.name),
		_mode(ci.mode),
		_index(ci.default_index),
		_same_line(ci.same_line)
	{
		if (ci.options.empty())
		{
			_options.resize(ci.labels.size());
			for (size_t i = 0; i < _options.size(); ++i)
			{
				_options[i] = Option{
					.name = ci.labels[i],
					.desc = {},
				};
			}
		}
		else
		{
			_options = ci.options;
		}
		if (_index > _options.size())	_index = 0;
		validate();
	}

	int ImGuiListSelection::validate()
	{
		int res = 0;
		for (size_t i = 0; i < _options.size(); ++i)
		{
			if (_options[i].name.empty())
			{
				_options[i].name = "##";
				res = std::max(res, 1);
			}
		}
		if (res != 0)
		{
			VKL_BREAKPOINT_HANDLE;
		}
		return res;
	}

	void ImGuiListSelection::setOptionsCount(uint32_t count)
	{
		_options.resize(count);
	}

	void ImGuiListSelection::setOption(size_t index, Option&& option)
	{
		if (_options.size() <= index)
		{
			setOptionsCount(index + 1);
		}
		_options[index] = std::move(option);
	}

	void ImGuiListSelection::setOption(size_t index, OptionView const& option)
	{
		if (_options.size() <= index)
		{
			setOptionsCount(index + 1);
		}
		_options[index].name = option.name;
		_options[index].desc = option.desc;
		_options[index].disable = option.disable;
	}

	bool ImGuiListSelection::declareRadioButtons(const char * name, size_t &active_index, bool same_line) const
	{
		if (name)
		{
			ImGui::Text(name);
			if (same_line)
			{
				ImGui::SameLine();
			}
		}
		const size_t old_index = active_index;
		for (size_t i = 0; i < _options.size(); ++i)
		{
			ImGui::BeginDisabled(_options[i].disable);
			const bool b = ImGui::RadioButton(_options[i].name.c_str(), i == old_index);
			if (!_options[i].desc.empty())
			{
				ImGui::SetItemTooltip(_options[i].desc.c_str());
			}
			if (b)	active_index = i;
			if (same_line && (i != _options.size() - 1))
				ImGui::SameLine();
			ImGui::EndDisabled();
		}
		bool changed = old_index != active_index;
		return changed;
	}

	bool ImGuiListSelection::declareCombo(const char * name, size_t &active_index) const
	{
		bool res = false;
		assert(name);
		bool begin = false;
		if (active_index < _options.size())
		{
			begin = ImGui::BeginCombo(name, _options[active_index].name.c_str());
		}
		else
		{
			constexpr const size_t max_size = 64;
			char preview_label[max_size];
			size_t write_count = std::format_to_n(preview_label, max_size, "option {} (unknown)", active_index).size;
			assert(write_count < max_size);
			preview_label[write_count] = char(0);
			begin = ImGui::BeginCombo(name, preview_label);
		}
		if (begin)
		{
			const size_t old_index = active_index;
			for (size_t i = 0; i < _options.size(); ++i)
			{
				ImGui::BeginDisabled(_options[i].disable);
				const bool b = old_index == i;
				if (ImGui::Selectable(_options[i].name.c_str(), b))
				{
					active_index = i;
				}
				if (b)
				{
					ImGui::SetItemDefaultFocus();
				}
				if (!_options[i].desc.empty())
				{
					ImGui::SetItemTooltip(_options[i].desc.c_str());
				}
				ImGui::EndDisabled();
			}
			ImGui::EndCombo();
			res = old_index != active_index;
		}
		return res;
	}

	bool ImGuiTransform3D::declare()
	{
		bool changed = false;

		ImGui::Checkbox("Raw Matrix", &_raw_view);
		
		ImGui::BeginDisabled(_read_only);
		
		if (_raw_view)
		{
			Matrix3x4f t = (*_matrix);
		
			for (size_t i = 0; i < 3; ++i)
			{
				char row_name[2];
				row_name[0] = 'x' + i;
				row_name[1] = 0;
				changed |= ImGui::DragFloat4(row_name, t.row(i).data(), 0.1);
			}
		}
		else
		{
			auto t = _matrix->col(3);
			Vector3f t_ = t;
			changed |= ImGui::DragFloat3("Translation", t_.data(), 0.1);
			if (changed)
			{
				t_ = t;
			}

			//bool changed2 = false;

			//glm::vec3 scale(
			//	glm::length((*_matrix)[0]),
			//	glm::length((*_matrix)[1]),
			//	glm::length((*_matrix)[2])
			//);
			//glm::mat3 rotation = glm::mat3(*_matrix);
			//rotation[0] /= scale[0];
			//rotation[1] /= scale[1];
			//rotation[2] /= scale[2];
			//glm::vec3 angles;

			//

			//changed2 |= ImGui::DragFloat3("Scale", &scale.x, 0.1);
		}

		if (!_read_only)
		{
			if (ImGui::Button("Reset"))
			{
				changed = true;
				*_matrix = DiagonalMatrix<3, 4>(1.0f);
			}
		}


		ImGui::EndDisabled();
		return changed;

	}
}


namespace ImGui
{
	bool DragAngle(const char* label, float* v_rad, float v_speed, float v_degrees_min, float v_degrees_max, const char* format, ImGuiSliderFlags flags)
	{
		if (format == NULL)
			format = "%.0f deg";
		float v_deg = (*v_rad) * 180.0f / std::numbers::pi;
		bool value_changed = DragFloat(label, &v_deg, v_speed, v_degrees_min, v_degrees_max, format, flags);
		if (value_changed)
			*v_rad = v_deg * std::numbers::pi / 180.0f;
		return value_changed;
	}

	bool SliderAngleN(const char* label, float* v_rad, uint N, float v_degrees_min, float v_degrees_max, const char* format, ImGuiSliderFlags flags, uint8_t* changed_bit_field)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		if (format == NULL)
			format = "%.0f deg";
		if (changed_bit_field)
		{
			std::memset(changed_bit_field, 0, std::divUpAssumeNoOverflow<uint>(N, 8));
		}

		ImGuiContext& g = *GetCurrentContext();
		bool value_changed = false;
		BeginGroup();
		PushID(label);
		PushMultiItemsWidths(N, CalcItemWidth());
		for (uint i = 0; i < N; i++)
		{
			PushID(i);
			if (i > 0)
				SameLine(0, g.Style.ItemInnerSpacing.x);
			bool changed = false;
			changed = SliderAngle("", v_rad + i, v_degrees_min, v_degrees_max, format, flags);
			value_changed |= changed;
			if(changed && changed_bit_field)
			{
				changed_bit_field[i / 8] |= uint8_t(1 << (i % 8));
			}
			PopID();
			PopItemWidth();
		}
		PopID();

		const char* label_end = FindRenderedTextEnd(label);
		if (label != label_end)
		{
			SameLine(0, g.Style.ItemInnerSpacing.x);
			TextEx(label, label_end);
		}

		EndGroup();
		return value_changed;
	}

	bool IconButtonEx(const char* str_id, ImVec2 size, ImGuiButtonFlags flags, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

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

		render_frame_fn(render_frame_data, window->DrawList, bb, g.FontSize, text_col);

		IMGUI_TEST_ENGINE_ITEM_INFO(id, str_id, g.LastItemData.StatusFlags);
		return pressed;
	}

	void DrawRectNoCorners(ImDrawList* draw_list, ImVec2 top_left, ImVec2 bottom_right, ImU32 color, float thickness)
	{
		// top
		draw_list->AddLine(ImVec2(top_left.x + 1, top_left.y), ImVec2(bottom_right.x, top_left.y), color, thickness);
		// bottom
		draw_list->AddLine(ImVec2(top_left.x + 1, bottom_right.y), ImVec2(bottom_right.x, bottom_right.y), color, thickness);
		// left
		draw_list->AddLine(ImVec2(top_left.x, top_left.y + 1), ImVec2(top_left.x, bottom_right.y), color, thickness);
		// right
		draw_list->AddLine(ImVec2(bottom_right.x, top_left.y + 1), ImVec2(bottom_right.x, bottom_right.y), color, thickness);
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
		DrawRectNoCorners(draw_list, pos - (detachment + ImVec2(1, 1)), pos + size - (detachment + ImVec2(1, 1)), color, thickness);
		DrawRectNoCorners(draw_list, pos + detachment, pos + size + detachment, color, thickness);
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

	bool DetachButton()
	{
		bool res = IconButton("Detach", RenderDetachIcon);
		//bool res = ImGui::ArrowButton("Detach", ImGuiDir_Up);
		ImGui::SetItemTooltip("Detach panel");
		return res;
	}

	bool XCrossButton(const char* tooltip)
	{
		bool res = IconButton("XCross", RenderXCrossIcon);
		if (tooltip)
		{
			ImGui::SetItemTooltip(tooltip);
		}
		return res;
	}

	int PushReadOnlyDisabledStyleCol(ImGuiCol_ color_id)
	{
		ImVec4 col = ImGui::GetStyleColorVec4(color_id);
		col.w *= ImGui::GetStyle().DisabledAlpha;
		ImGui::PushStyleColor(color_id, col);
		return 1;
	}

	void LabelText2(const char* label, const char* text, ImGuiInputTextFlags flags)
	{
		static std::string local_string;
		local_string = text;
		int pushed = PushReadOnlyDisabledStyleCol();
		ImGui::InputText(label, &local_string, flags | ImGuiInputTextFlags_ReadOnly);
		ImGui::PopStyleColor(pushed);
	}

	void LabelValueEx(const char* label, ImGuiDataType type, const void* value, const char* fmt)
	{
		int pushed = PushReadOnlyDisabledStyleCol();
		ImGui::InputScalar(label, type, const_cast<void*>(value), nullptr, nullptr, fmt, ImGuiInputTextFlags_ReadOnly);
		ImGui::PopStyleColor(pushed);
	}

	void LabelCheckbox(const char* label, bool b)
	{
		if (false)
		{
			BeginDisabled();
			Checkbox(label, &b);
			EndDisabled();
			return;
		}

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);

		const float square_sz = GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
		ItemSize(total_bb, style.FramePadding.y);
		const bool is_visible = ItemAdd(total_bb, id);
		if (!is_visible)
		{
			return;
		}

		const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
		if (is_visible)
		{
			RenderFrame(check_bb.Min, check_bb.Max, GetColorU32(ImGuiCol_FrameBg, g.Style.DisabledAlpha), true, style.FrameRounding);
			if (b)
			{
				ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
				const float pad = ImMax(1.0f, IM_TRUNC(square_sz / 6.0f));
				RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
			}
		}
		const ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
		if (g.LogEnabled)
			LogRenderedText(&label_pos, b ? "[x]" : "[ ]");
		if (is_visible && label_size.x > 0.0f)
			RenderText(label_pos, label);
	}

	bool TextFieldEdit(const char* label, std::string* str, const char* hint, ImGuiInputTextFlags flags)
	{
		// It would be nice to have a nice icon (a lens if the field is empty)
		ImGui::SetNextItemAllowOverlap();
		bool res = ImGui::InputTextWithHint(label, "Filter...", str, flags);
		
		if (!str->empty())
		{
			const ImVec2 save_pos = ImGui::GetCursorPos();

			ImGui::SameLine();
			float padding = std::ceil(ImGui::GetFontSize() / 16.0f);
			ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(ImGui::CalcTextSize(label).x + 3 * ImGui::GetStyle().ItemInnerSpacing.x + ImGui::GetFrameHeight() - padding, -padding));

			float button_size = (ImGui::GetFrameHeight() - 2 * padding);
			if (ImGui::IconButtonEx("Clear", ImVec2(button_size, button_size), ImGuiButtonFlags_None, ImGui::RenderXCrossIcon))
			{
				str->clear();
				res = true;
			}
			ImGui::SetCursorPos(save_pos);
		}
		
		return res;
	}
}