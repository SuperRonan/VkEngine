#define IMGUI_DEFINE_MATH_OPERATORS 1

#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/Context.hpp>
#include <cassert>

#include <vkl/Maths/Transforms.hpp>

#include <numbers>

#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <span>

#include <vkl/GUI/FancyButtons.hpp>

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

	

	struct DeclareTextFieldInlineButtons
	{
		using DeclareFn = void(*)(void* data, ImVec2 button_size);
		DeclareFn declare;
		void* data;
	};

	bool TextFieldEditEx(const char* label, std::string* str, const char* hint, ImGuiInputTextFlags flags, std::span<DeclareTextFieldInlineButtons> extra_buttons = {})
	{
		// It would be nice to have a nice icon (a lens if the field is empty)
		ImGui::SetNextItemAllowOverlap();
		bool res = ImGui::InputTextWithHint(label, hint, str, flags);
		
		if (!str->empty() || !extra_buttons.empty())
		{
			const ImVec2 save_pos = ImGui::GetCursorPos();

			ImGui::SameLine();
			const ImVec2 save_line_pos = ImGui::GetCursorPos();
			float padding = std::ceil(ImGui::GetFontSize() / 16.0f);
			float button_size = (ImGui::GetFrameHeight() - 2 * padding);
			const float interspace = ImGui::GetStyle().ItemInnerSpacing.x;
			const float button_spacing = button_size + padding;
			float button_count = extra_buttons.size();
			if (!str->empty())
			{
				++button_count;
			}
			const ImVec2 base_pos = ImGui::GetCursorPos() - ImVec2(ImGui::CalcTextSize(label).x + 3 * interspace + button_spacing * button_count, 0);
			const ImVec2 button_size2 = ImVec2(button_size, button_size);
			uint button_counter = 0;
			if (!str->empty())
			{
				ImGui::SetCursorPos(base_pos + ImVec2(button_counter++ * button_spacing, padding));
				if (ImGui::IconButtonEx("Clear", button_size2, ImGuiButtonFlags_None, ImGui::RenderXCrossIcon))
				{
					str->clear();
					res = true;
				}
				ImGui::SameLine();
			}

			for (size_t i = 0; i < extra_buttons.size(); ++i)
			{
				ImGui::SetCursorPos(base_pos + ImVec2(button_counter++ * button_spacing, padding));
				extra_buttons[i].declare(extra_buttons[i].data, button_size2);
				ImGui::SameLine();
			}

			ImGui::SetCursorPos(save_line_pos + ImVec2(- 2 * interspace, 0));
			ImGui::NewLine();
			ImGui::SetCursorPos(save_pos);
		}
		
		return res;
	}

	bool TextFieldEdit(const char* label, std::string* str, const char* hint, ImGuiInputTextFlags flags)
	{
		return TextFieldEditEx(label, str, hint, flags);
	}

	bool DeclareFilter(std::string* str, bool* p_case_sensitive, ImGuiInputTextFlags flags)
	{
		std::array<DeclareTextFieldInlineButtons, 1> extra_buttons = {};
		uint extra_button_count = 0;
		struct CaseSensitiveData
		{
			bool* ptr = {};
			bool res = false;
		};
		CaseSensitiveData case_sensitive_data;
		if (p_case_sensitive)
		{
			case_sensitive_data = {
				.ptr = p_case_sensitive,
			};
			extra_buttons[extra_button_count++] = DeclareTextFieldInlineButtons{
				.declare = [](void* p_data, ImVec2 size)
				{
					CaseSensitiveData* data = reinterpret_cast<CaseSensitiveData*>(p_data);
					data->res |= ImGui::InboxCheckbox("Aa", data->ptr, size);
					ImGui::SetItemTooltip(*data->ptr ? "De-Activate Case Sensitive" : "Activate Case Sensitive");
				},
				.data = &case_sensitive_data,
			};
		}
		bool res = TextFieldEditEx("Filter", str, "Filter...", flags, std::span(extra_buttons.data(), extra_button_count));
		res |= case_sensitive_data.res;
		return res;
	}

	void CannotAcceptDragDropPayload(const char* reason)
	{
		ImVec4 background = ImVec4(0.333, 0, 0, 0.4);
		ImVec4 border = ImVec4(0.5, 0, 0, 0.8);
		ImGuiContext& g = *GImGui;
		{
			ImRect r = g.DragDropTargetRect;
			ImGui::PushStyleColor(ImGuiCol_DragDropTargetBg, background);
			ImGui::PushStyleColor(ImGuiCol_DragDropTarget, border);
			ImGui::RenderDragDropTargetRectForItem(r);
			ImGui::PopStyleColor(2);
		}
		if (reason)
		{
			if (true)
			{
				const bool color_txt = true;
				// If drag and drop already active (wich should be the case)
				// BeginTooltip would overwrite the drag tooltip if g.DragDropWithinTarget is true
				// => Set it to false to append to a "regular" tooltip
				bool push_tg = g.DragDropWithinTarget;
				g.DragDropWithinTarget = false;
				if (push_tg)
				{
					// Position the regular tooltip after the drag tooltip (not perfect, the positioning could be improved)
					const char* window_name_template = "##Tooltip_DragDrop_%02d";
					char window_name[32];
					ImFormatString(window_name, IM_COUNTOF(window_name), window_name_template, g.TooltipOverrideCount);
					ImGuiWindow* drag_tooltip_window = ImGui::FindWindowByName(window_name);
					ImGui::SetNextWindowPos(drag_tooltip_window->Rect().GetBL(), ImGuiCond_None);
					if (!color_txt)
					{
						ImGui::PushStyleColor(ImGuiCol_PopupBg, background);
						ImGui::PushStyleColor(ImGuiCol_Border, border);
					}
				}
				if (ImGui::BeginTooltip())
				{
					if (color_txt)
					{
						ImGui::TextColored(border, reason);
					}
					else
					{
						ImGui::TextUnformatted(reason);
					}
					ImGui::EndTooltip();
				}
				if (push_tg)
				{
					if (!color_txt)
					{
					ImGui::PopStyleColor(2);
					}
				}
				g.DragDropWithinTarget = push_tg;
			}
			else
			{
				ImGui::SetTooltip(reason);
			}
		}
		ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
	}

	void SignalDragDropTarget(bool valid)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		float pulse = std::sqr(std::sin(g.Time * std::numbers::pi * 0.75));
		vkl::Vector3f color = valid ? vkl::Vector3f(1, 1, 1) : vkl::Vector3f(0.5, 0, 0);
		ImVec4 background = ImVec4(color.x(), color.y(), color.z(), std::lerp(0.05, 0.15, pulse));
		ImVec4 border = ImVec4(color.x(), color.y(), color.z(), std::lerp(0.2, 0.4, pulse));
		const ImRect display_rect = (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasDisplayRect) ? g.LastItemData.DisplayRect : g.LastItemData.Rect;
		auto dnd_clip_rect = g.DragDropTargetClipRect; // Probably not necessary
		g.DragDropTargetClipRect = (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasClipRect) ? g.LastItemData.ClipRect : window->ClipRect; // Used by ImGui::RenderDragDropTargetRectForItem(), but not set because outisde of a ImGui::BeginDragDropTarget()
		ImGui::PushStyleColor(ImGuiCol_DragDropTargetBg, background);
		ImGui::PushStyleColor(ImGuiCol_DragDropTarget, border);
		ImGui::RenderDragDropTargetRectForItem(display_rect);
		ImGui::PopStyleColor(2);
		g.DragDropTargetClipRect = dnd_clip_rect;
	}

	bool AppendingToAlreadyBegan()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		// Good enough for now, improve IFN
		bool res = window->DC.CursorPos.y > window->DC.CursorStartPos.y;
		return res;
	}

	ImRect GetItemRect()
	{
		ImGuiContext& g = *GImGui;
		return g.LastItemData.Rect;
	}

	float GetFrameWidth()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		return window->InnerClipRect.GetWidth();
	}

	void CenterNextItem(float expected_width)
	{
		float w = GetFrameWidth();
		SetCursorPosX(GetCursorPosX() + ImFloor(0.5f * (w - expected_width)));
	}

}

namespace vkl::GUI
{
	int SectionBox::pushStyleColor()
	{
		ImGui::PushStyleColor(ImGuiCol_Border, _color.Value);
		ImGui::PushStyleColor(ImGuiCol_Separator, _color.Value);
		return 2;
	}

	bool SectionBox::begin(Context& ctx)
	{
		_should_pop_item_width = false;
		int color_push = 0;
		if (stack_color)
		{
			_color = ctx.pushStack();
			color_push += pushStyleColor();
		}
		const float top_item_width = ImGui::GetCurrentWindow()->DC.ItemWidth;
		bool res = ImGui::BeginChild(label, ImVec2(0, 0), child_flags, window_flags);
		if (res)
		{
			ImGui::SeparatorText(label);
			if(full_width)
			{
				const auto& style = ImGui::GetStyle();
				float item_width = top_item_width - 2 * style.FramePadding.x - style.FrameBorderSize;
				ImGui::PushItemWidth(item_width);
				_should_pop_item_width = true;
			}
		}
		if (color_push)
		{
			ImGui::PopStyleColor(color_push);
			color_push = 0;
		}
		return res;
	}

	void SectionBox::end(Context& ctx)
	{
		if (_should_pop_item_width)
		{
			ImGui::PopItemWidth();
			_should_pop_item_width = false;
		}
		int color_push = 0;
		if (stack_color)
		{
			color_push += pushStyleColor();
		}
		ImGui::EndChild();
		if (color_push)
		{
			ImGui::PopStyleColor(color_push);
			color_push = 0;
		}
		if (stack_color)
		{
			ctx.popStack();
		}
	}
}