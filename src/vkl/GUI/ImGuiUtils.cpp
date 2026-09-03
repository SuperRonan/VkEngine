#define IMGUI_DEFINE_MATH_OPERATORS 1

#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/Context.hpp>
#include <cassert>

#include <vkl/Maths/Transforms.hpp>
#include <vkl/Maths/Types.hpp>

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
					.label = ci.labels[i],
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
			if (_options[i].label.empty())
			{
				_options[i].label = "##";
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
		_options[index].label = option.label;
		_options[index].desc = option.desc;
		_options[index].disable = option.disable;
	}

	template <that::concepts::StringLike Str>
	int ImGuiListSelection::DeclareRadioButtons(DeclareInfoT<Str> const& info)
	{
		int res = -1;
		if (info.label)
		{
			ImGui::Text(info.label);
			if (info.same_line)
			{
				ImGui::SameLine();
			}
		}
		for (uint i = 0; i < info.options.size(); ++i)
		{
			const auto& option = info.options[i];
			ImGui::BeginDisabled(option.disable);
			std::string_view option_label = that::StringViewMaybeNull(option.label);
			const bool b = ImGui::RadioButton(option_label.data(), i == info.index);
			std::string_view option_desc = that::StringViewMaybeNull(option.desc);
			if (!option_desc.empty())
			{
				ImGui::SetItemTooltip(option_desc.data());
			}
			if (b)	res = i;
			if (info.same_line && (i != info.options.size() - 1))
				ImGui::SameLine();
			ImGui::EndDisabled();
		}
		return res;
	}

	template int ImGuiListSelection::DeclareRadioButtons(DeclareInfoT<std::string> const& info);
	template int ImGuiListSelection::DeclareRadioButtons(DeclareInfoT<std::string_view> const& info);
	template int ImGuiListSelection::DeclareRadioButtons(DeclareInfoT<const char*> const& info);

	template <that::concepts::StringLike Str>
	int ImGuiListSelection::DeclareCombo(DeclareInfoT<Str> const& info)
	{
		int res = -1;
		bool begin = false;
		if (info.index < info.options.size())
		{
			std::string_view option_label = that::StringViewMaybeNull(info.options[info.index].label);
			begin = ImGui::BeginCombo(info.label, option_label.data());
		}
		else
		{
			constexpr const size_t max_size = 64;
			char preview_label[max_size];
			size_t write_count = std::format_to_n(preview_label, max_size, "option {} (unknown)", info.index).size;
			assert(write_count < max_size);
			preview_label[write_count] = char(0);
			begin = ImGui::BeginCombo(info.label, preview_label);
		}
		if (begin)
		{
			for (uint i = 0; i < info.options.size(); ++i)
			{
				ImGui::BeginDisabled(info.options[i].disable);
				const bool b = info.index == i;
				std::string_view option_label = that::StringViewMaybeNull(info.options[i].label);
				if (ImGui::Selectable(option_label.data(), b))
				{
					res = i;
				}
				if (b)
				{
					ImGui::SetItemDefaultFocus();
				}
				std::string_view option_desc = that::StringViewMaybeNull(info.options[i].desc);
				if (!option_desc.empty())
				{
					ImGui::SetItemTooltip(option_desc.data());
				}
				ImGui::EndDisabled();
			}
			ImGui::EndCombo();
		}
		return res;
	}

	template int ImGuiListSelection::DeclareCombo(DeclareInfoT<std::string> const& info);
	template int ImGuiListSelection::DeclareCombo(DeclareInfoT<std::string_view> const& info);
	template int ImGuiListSelection::DeclareCombo(DeclareInfoT<const char*> const& info);

	bool ImGuiListSelection::declareRadioButtons(const char * name, size_t &active_index, bool same_line) const
	{
		DeclareInfo info{
			.label = name,
			.options = _options,
			.index = uint(active_index),
			.same_line = same_line,
		};
		int index = DeclareRadioButtons(info);
		bool res = index >= 0;
		if (res)
		{
			active_index = index;
		}
		return res;
	}

	bool ImGuiListSelection::declareCombo(const char * name, size_t &active_index) const
	{
		DeclareInfo info{
			.label = name,
			.options = _options,
			.index = uint(active_index),
		};
		int index = DeclareCombo(info);
		bool res = index >= 0;
		if (res)
		{
			active_index = index;
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
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		float w = window->ContentRegionRect.GetWidth();
		SetCursorPosX(GetCursorPosX() + ImFloor(0.5f * (w - expected_width)));
	}

	struct IdHelper
	{
		bool pushed = false;

		IdHelper() = default;

		IdHelper(const char* label, const char* end = nullptr)
		{
			PushID(label, end);
			pushed = true;
		}

		IdHelper(const void* ptr)
		{
			PushID(ptr);
			pushed = true;
		}

		IdHelper(int n)
		{
			PushID(n);
			pushed = true;
		}

		~IdHelper()
		{
			if (pushed)
			{
				PopID();
			}
		}
	};


	// User can input math operators (e.g. +100) to edit a numerical values.
	// NB: This is _not_ a full expression evaluator. We should probably add one and replace this dumb mess..
	// Returns the number of chars that were read if successful
	int DataTypeApplyFromText2(const char* buf, ImGuiDataType data_type, void* p_data, const char* format)
	{
		const char* const _buf = buf;
		// Copy the value in an opaque buffer so we can compare at the end of the function if it changed at all.
		const ImGuiDataTypeInfo* type_info = DataTypeGetInfo(data_type);
		ImGuiDataTypeStorage data_backup;
		memcpy(&data_backup, p_data, type_info->Size);
		buf = ImStrSkipBlank(buf);
		if (!buf[0])
		{
			return -1;
		}
		int res = static_cast<int>(buf - _buf);

		// Sanitize format
		// - For float/double we have to ignore format with precision (e.g. "%.2f") because sscanf doesn't take them in, so force them into %f and %lf
		// - In theory could treat empty format as using default, but this would only cover rare/bizarre case of using InputScalar() + integer + format string without %.
		char format_sanitized[32];
		if (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double)
			format = type_info->ScanFmt;
		else
			format = ImParseFormatSanitizeForScanning(format, format_sanitized, IM_COUNTOF(format_sanitized));
		size_t format_len = strlen(format);
		std::memmove(format_sanitized, format, format_len);
		IM_ASSERT(format_len + 2 < IM_COUNTOF(format_sanitized));
		strcpy(format_sanitized + format_len, "%n");
		format_len += 2;

		// Small types need a 32-bit buffer to receive the result from scanf()
		int v32 = 0;
		int read_chars = 0;
		int scan_res = sscanf(buf, format_sanitized, type_info->Size >= 4 ? p_data : &v32, &read_chars);
		if (scan_res < 1)
		{
			return -1;
		}
		res += read_chars;
		if (type_info->Size < 4)
		{
			if (data_type == ImGuiDataType_S8)
				*(ImS8*)p_data = (ImS8)ImClamp(v32, (int)std::numeric_limits<ImS8>::min(), (int)std::numeric_limits<ImS8>::max());
			else if (data_type == ImGuiDataType_U8)
				*(ImU8*)p_data = (ImU8)ImClamp(v32, (int)std::numeric_limits<ImU8>::min(), (int)std::numeric_limits<ImU8>::max());
			else if (data_type == ImGuiDataType_S16)
				*(ImS16*)p_data = (ImS16)ImClamp(v32, (int)std::numeric_limits<ImS16>::min(), (int)std::numeric_limits<ImS16>::max());
			else if (data_type == ImGuiDataType_U16)
				*(ImU16*)p_data = (ImU16)ImClamp(v32, (int)std::numeric_limits<ImS16>::min(), (int)std::numeric_limits<ImU16>::max());
			else
				IM_ASSERT(0);
		}
		return res;
	}

	// Returns error
	const char* TryParseRangeText(
		ImGuiDataType data_type,
		const char* txt,
		void* range_min, void* range_max,
		const void* p_lower_bound, const void* p_upper_bound,
		const char* format,
		bool skip_clamp_bounds,
		InputRangeRes& res,
		ImGuiDataTypeStorage* parse_data,
		const char*& error_extra
	) {
		std::string_view txt_sv(txt);
		size_t separator_pos = txt_sv.find_first_of(":;~");
		if (separator_pos == std::string_view::npos)
		{
			return "Cannot find separator (must be one of {':', ';', '~'}).";
		}
		res.separator = txt_sv[separator_pos];
		std::string_view txt_min(txt, separator_pos);
		std::string_view txt_max(txt + separator_pos + 1, txt_sv.size() - separator_pos - 1);

		auto ParseBound = [&](std::string_view txt, void* dst) -> char {
			bool blank = std::all_of(txt.begin(), txt.end(), ImCharIsBlankA);
			if (blank)
			{
				return ' ';
			}
			// test if _
			{
				uint count__ = 0;
				for (const char* reader = txt.data(); reader != txt.data() + txt.size(); ++reader)
				{
					if (*reader == '_')
					{
						++count__;
					}
					else if (!ImCharIsBlankA(*reader))
					{
						break;
					}
				}
				if (count__)
				{
					return '_';
				}
			}
			int parse_res = DataTypeApplyFromText2(txt.data(), data_type, dst, format);
			if (parse_res < 1)
			{
				return char(0);
			}
			return '1';
			};

		auto ParseBound2 = [&](std::string_view txt, ImGuiDataTypeStorage& dst, const void* current_value, const void* p_blank_value, char& tag) -> const char*
			{
				char parse_res = ParseBound(txt, &dst);
				tag = parse_res;
				if (parse_res == ' ')
				{
					if (p_blank_value)
					{
						dst = *reinterpret_cast<const ImGuiDataTypeStorage*>(p_blank_value);
					}
					else
					{
						return "Undefined blank value";
					}
				}
				else if (parse_res == '_')
				{
					dst = *reinterpret_cast<const ImGuiDataTypeStorage*>(current_value);
				}
				else if (parse_res == '1')
				{

				}
				else
				{
					return "Failed to parse value";
				}
				return nullptr;
			};

		const char* parse_error = nullptr;
		parse_error = ParseBound2(txt_min, parse_data[0], range_min, p_lower_bound, res.min_tag);
		if (parse_error)
		{
			error_extra = " (lower bound).";
			return parse_error;
		}
		parse_error = ParseBound2(txt_max, parse_data[1], range_max, p_upper_bound, res.max_tag);
		if (parse_error)
		{
			error_extra = " (upper bound).";
			return parse_error;
		}

		{
			alignas(size_t) ImGuiDataTypeStorage len_data;
			auto compute_len = [&]() {DataTypeApplyOp(data_type, '-', &len_data, &parse_data[1], &parse_data[0]); };
			if (res.separator == ':')
			{
				int cmp = DataTypeCompare(data_type, &parse_data[0], &parse_data[1]);
				if (cmp >= 1)
				{
					ImSwap(parse_data[0], parse_data[1]);
				}
			}
			else if (res.separator == ';')
			{
				len_data = parse_data[1];
				DataTypeApplyOp(data_type, '+', &parse_data[1], &parse_data[0], &len_data);
			}
			else if (res.separator == '~')
			{
				len_data = parse_data[0];
				DataTypeApplyOp(data_type, '-', &parse_data[0], &parse_data[1], &len_data);
			}
			else
			{
				return "Unhandled separator.";
			}

			if (!skip_clamp_bounds)
			{
				if (p_lower_bound || p_upper_bound)
				{
					DataTypeClamp(data_type, &parse_data[0], p_lower_bound, p_upper_bound);
					DataTypeClamp(data_type, &parse_data[1], p_lower_bound, p_upper_bound);
				}
			}

			return nullptr;
		}
	}

	InputRangeRes InputRangeExImpl(
		const char* label,
		ImRect const& frame,
		ImGuiID id,
		ImGuiDataType data_type,
		size_t data_type_size,
		void* range_min, // != nullptr
		void* range_max, // != nullptr
		const void* p_lower_bound,
		const void* p_upper_bound,
		const char* format,
		bool skip_clamp_bounds,
		ImGuiInputTextFlags flags
	) {
		InputRangeRes res = {};

		if (data_type_size == 0)
		{
			data_type_size = DataTypeGetInfo(data_type)->Size;
		}

		ImGuiContext& g = *GImGui;
		char data_buff[128];
		char* p_txt_data = data_buff;
		ImGuiInputTextFlags txt_flags = flags | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_LocalizeDecimalPoint | ImGuiInputTextFlags_EnterReturnsTrue;
		char range_format_buf[64];
		const std::string_view fmt_sv = format;
		char* p_range_format = range_format_buf;
		assert(strlen(format) * 2 + 1 < IM_COUNTOF(range_format_buf));
		auto append = [](char*& dst, const char* src)
			{
				size_t n = strlen(src);
				strcpy(dst, src);
				dst += n;
			};
		append(p_range_format, format);
		append(p_range_format, ":");
		append(p_range_format, format);

		p_txt_data += DataTypeFormatString(p_txt_data, IM_COUNTOF(data_buff) - (p_txt_data - data_buff), data_type, range_min, format);
		append(p_txt_data, ":");
		p_txt_data += DataTypeFormatString(p_txt_data, IM_COUNTOF(data_buff) - (p_txt_data - data_buff), data_type, range_max, format);
		g.LastItemData.ItemFlags |= ImGuiItemFlags_NoMarkEdited; // Because TempInputText() uses ImGuiInputTextFlags_MergedItem it doesn't submit a new item, so we poke LastItemData.
		alignas(size_t) ImGuiDataTypeStorage parse_data[2];
		bool input_res = TempInputText(frame, id, label, data_buff, IM_COUNTOF(data_buff), txt_flags);
		if (input_res)
		{
			VKL_BREAKPOINT_HANDLE;
		}

		// Attempt to parse
		const char* error_extra = nullptr;
		const char* error = TryParseRangeText(data_type, data_buff, range_min, range_max, p_lower_bound, p_upper_bound, format, skip_clamp_bounds, res, parse_data, error_extra);

		if (error)
		{
			if (ImGui::BeginErrorTooltip())
			{
				if (error_extra)
				{
					ImGui::Text("%s%s", error, error_extra);
				}
				else
				{
					ImGui::TextUnformatted(error);
				}
				ImGui::EndErrorTooltip();
			}
		}

		g.LastItemData.ItemFlags &= ~ImGuiItemFlags_NoMarkEdited;
		if (!error && input_res)
		{
			if (memcmp(range_min, &parse_data[0], data_type_size) != 0)
			{
				res.flags |= res.EDIT_MIN_BIT;
				memcpy(range_min, &parse_data[0], data_type_size);
			}
			if (memcmp(range_max, &parse_data[1], data_type_size) != 0)
			{
				res.flags |= res.EDIT_MAX_BIT;
				memcpy(range_max, &parse_data[1], data_type_size);
			}
			res.flags |= res.INPUT_TEXT_BIT;
			MarkItemEdited(id);
		}
		return res;
	}

	//InputRangeRes InputRangeEx(
	//	const char* label,
	//	ImGuiDataType data_type,
	//	size_t data_type_size,
	//	void* range_min, // != nullptr
	//	void* range_max, // != nullptr
	//	const void* p_lower_bound,
	//	const void* p_upper_bound,
	//	const char* format,
	//	bool skip_clamp_bounds,
	//	ImGuiInputTextFlags flags
	//) {
	//	ImGuiWindow* window = GetCurrentWindow();
	//	if (window->SkipItems)
	//		return {};

	//	ImGuiContext& g = *GImGui;
	//	ImGuiStyle& style = g.Style;

	//	if (format == NULL)
	//		format = DataTypeGetInfo(data_type)->PrintFmt;

	//	ImGuiID id = window->GetID(label);
	//	const float w = CalcItemWidth();
	//	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	//	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
	//	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	//	ItemSize(total_bb, style.FramePadding.y);
	//	if (!ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
	//	{
	//		return {};
	//	}

	//	const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
	//	bool temp_input_active = TempInputIsActive(id);
	//	if (!temp_input_active)
	//	{
	//		const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
	//		const bool double_clicked = (hovered && g.IO.MouseClickedCount[0] == 2 && TestKeyOwner(ImGuiKey_MouseLeft, id));
	//		const bool make_active = (clicked || double_clicked || g.NavActivateId == id);
	//	}

	//	InputRangeRes res = InputRangeExImpl(
	//		label,
	//		frame_bb,
	//		id,
	//		data_type,
	//		0,
	//		range_min, range_max,
	//		p_lower_bound, p_upper_bound,
	//		format,
	//		skip_clamp_bounds,
	//		flags
	//	);

	//	return res;
	//}

	template <class T>
	T CenterInside(T outer_size, T inner_size)
	{
		return (outer_size - inner_size) / 2;
	}

	ImVec2 CenterRectPos(ImRect const& outer, ImVec2 inner_size)
	{
		return outer.Min + CenterInside(outer.GetSize(), inner_size);
	}

	ImRect CenterRect(ImRect const& outer, ImVec2 inner_size)
	{
		ImRect res;
		res.Min = CenterRectPos(outer, inner_size);
		res.Max = res.Min + inner_size;
		return res;
	}

	ImVec2 CalcDeltaToFitInside(ImRect const& outer, ImRect const& inner, bool center_if_bigger=false)
	{
		ImVec2 min_d = inner.Min - outer.Min;
		ImVec2 max_d = inner.Max - outer.Max;
		ImVec2 res = ImVec2(0, 0);
		for (uint i = 0; i < 2; ++i)
		{
			const bool move_lower = min_d[i] < 0;
			const bool move_upper = max_d[i] > 0;
			if (move_lower && move_upper)
			{
				if (center_if_bigger)
				{
					res[i] = (-min_d[i] - max_d[i]) * 0.5f;
				}
			}
			else if(move_lower)
			{
				res[i] = -min_d[i];
			}
			else if (move_upper)
			{
				res[i] = -max_d[i];
			}
		}
		return res;
	}

	ImRect FitRectInside(ImRect const& outer, ImRect const& inner, bool center_if_bigger = false)
	{
		ImRect res = inner;
		ImVec2 delta = CalcDeltaToFitInside(outer, inner, center_if_bigger);
		res.Translate(delta);
		return res;
	}

#define IMPORT_IMGUI_EXTERN_TEMPLATES(T, ST, FT) \
	extern template IMGUI_API float ScaleRatioFromValueT<T, ST, FT>(ImGuiDataType data_type, T v, T v_min, T v_max, float logarithmic_zero_epsilon, float zero_deadzone_size); \
	extern template IMGUI_API T     ScaleValueFromRatioT<T, ST, FT>(ImGuiDataType data_type, float t, T v_min, T v_max, float logarithmic_zero_epsilon, float zero_deadzone_size); \
	extern template IMGUI_API bool  DragBehaviorT<T, ST, FT>(ImGuiDataType data_type, T* v, float v_speed, T v_min, T v_max, const char* format, ImGuiSliderFlags flags); \
	extern template IMGUI_API bool  SliderBehaviorT<T, ST, FT>(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, T* v, T v_min, T v_max, const char* format, ImGuiSliderFlags flags, ImRect* out_grab_bb); \
	extern template IMGUI_API T     RoundScalarWithFormatT<T>(const char* format, ImGuiDataType data_type, T v); \
	extern template IMGUI_API bool  CheckboxFlagsT<T>(const char* label, T* flags, T flags_value);

	IMPORT_IMGUI_EXTERN_TEMPLATES(i32, i32, float)
	IMPORT_IMGUI_EXTERN_TEMPLATES(u32, i32, float)
	
	IMPORT_IMGUI_EXTERN_TEMPLATES(i64, i64, double)
	IMPORT_IMGUI_EXTERN_TEMPLATES(u64, i64, double)
	
	IMPORT_IMGUI_EXTERN_TEMPLATES(float, float, float)
	IMPORT_IMGUI_EXTERN_TEMPLATES(double, double, double)

	struct SliderRangeWidgetImpl
	{
		enum class Index : signed char
		{
			Min = 0,
			Inter = 1,
			Max = 2,
			_Count,
			None = -1,
		};
		using enum Index;

		float _min_grab_pos, _max_grab_pos; // in px
		float _grab_size; // in px
		//char _grabs_overlapping = {}; // 0: no overlapping, 1: overlapping, 2:merge
		bool _grab_ok = false;
		bool _hovered = {};
		bool _temp_input_allowed = {};
		bool _temp_input_active = {};
		bool _render_grabs = false;
		ImRect _frame = {};
		ImGuiID _id = {};
		ImGuiDataType _data_type = {};
		const ImGuiDataTypeInfo* _data_type_info = {};

		Index _hovered_index = Index::None;

		struct Globals
		{
			Index index = Index::None; // Active index
			alignas(uintptr_t) ImGuiDataTypeStorage backup_data[3];
		};
		// TODO move this to the context
		static Globals _g;

		static constexpr float _grab_padding = 2;
		static constexpr float DRAG_MOUSE_THRESHOLD_FACTOR = 0.5f;
		static constexpr ImGuiMouseButton slide_click_button = 0;

		// Raise the type to at least 32 bits
		template <std::arithmetic T>
		using Type32 = typename std::conditional<
			sizeof(T) < sizeof(int),
			typename std::conditional<std::is_signed<T>::value, int32_t, uint32_t>::type,
			T
		>::type;

		bool isActive() const
		{
			return GImGui->ActiveId == _id;
		}

		void clearActive()
		{
			ClearActiveID();
			_g.index = Index::None;
			std::memset(_g.backup_data, 0, sizeof(_g.backup_data));
		}

		template <std::arithmetic T>
		InputRangeRes behaviourT(T* range, const T* bounds, const T* len_bounds, const char* format, ImGuiSliderFlags flags)
		{
			_render_grabs = true;
			using Type32 = T;
			using SignedType = typename std::signed_type<Type32>::type;
			using FloatType = typename that::FloatTypePerSize<sizeof(Type32)>::type;

			constexpr const ImGuiDataType data_type = GetDataType<T>();
			T& range_min = range[0];
			T& range_max = range[1];
			T range_len = range_max - range_min;
			const T& lower_bound = bounds[0];
			const T& upper_bound = bounds[1];
			const T* min_len = len_bounds;
			const T* max_len = len_bounds ? len_bounds + 1 : nullptr;

			IM_ASSERT(lower_bound <= upper_bound);
			IM_ASSERT((!min_len || !max_len) || (*min_len <= *max_len));

			const ImRect& bb = _frame;

			ImGuiContext& g = *GImGui;
			const ImGuiStyle& style = g.Style;

			constexpr const bool is_floating_point = that::concepts::FloatingPoint<T>;
			const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
			const float v_range_f = ImAbs(float(upper_bound - lower_bound)); // We don't need high precision for what we do with it.
			const bool is_logarithmic = (flags & ImGuiSliderFlags_Logarithmic) != 0;

			const float grab_padding = _grab_padding;
			const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
			float grab_sz = style.GrabMinSize;
			if (!is_floating_point && v_range_f >= 0.0f)                         // v_range_f < 0 may happen on integer overflows
				grab_sz = ImMax(slider_sz / (v_range_f + 1), style.GrabMinSize); // For integer sliders: if possible have the grab size represent 1 unit
			grab_sz = ImMin(grab_sz, slider_sz);
			const float slider_usable_sz = slider_sz - grab_sz;
			const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
			const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

			float logarithmic_zero_epsilon = 0.0f; // Only valid when is_logarithmic is true
			float zero_deadzone_halfsize = 0.0f; // Only valid when is_logarithmic is true
			if (is_logarithmic)
			{
				// When using logarithmic sliders, we need to clamp to avoid hitting zero, but our choice of clamp value greatly affects slider precision. We attempt to use the specified precision to estimate a good lower bound.
				const int decimal_precision = is_floating_point ? ImParseFormatPrecision(format, 3) : 1;
				logarithmic_zero_epsilon = ImPow(0.1f, (float)decimal_precision);
				zero_deadzone_halfsize = (style.LogSliderDeadzone * 0.5f) / ImMax(slider_usable_sz, 1.0f);
			}

			auto RatioFromValue = [&](T value) -> float
			{
				return ScaleRatioFromValueT<Type32, SignedType, FloatType>(GetDataType<T>(), value, lower_bound, upper_bound, logarithmic_zero_epsilon, zero_deadzone_halfsize);
			};

			auto ValueFromRatio = [&](float t) -> T
			{
				Type32 res = ScaleValueFromRatioT<Type32, SignedType, FloatType>(GetDataType<T>(), t, lower_bound, upper_bound, logarithmic_zero_epsilon, zero_deadzone_halfsize);
				return static_cast<T>(res);
			};

			float grab_min_t = RatioFromValue(range_min);
			float grab_max_t = RatioFromValue(range_max);
			float mouse_abs_pos = g.IO.MousePos[axis];
			float rel_mouse_pos = mouse_abs_pos - (slider_usable_pos_min);
			float mouse_t = rel_mouse_pos / slider_usable_sz;
			if (axis == ImGuiAxis_Y)
			{
				grab_min_t = 1.0f - grab_min_t;
				grab_max_t = 1.0f - grab_max_t;
			}

			InputRangeRes res = {};

			T grab_min, grab_max;
			const float* grab_t = nullptr;
			T* slide_target_value = nullptr;

			float inter_clicked_t;
			T inter_value;

			auto select_target_value = [&]()
			{
				if (_g.index == Index::Min)
				{
					grab_min = lower_bound;
					grab_max = range_max;
					if (min_len)
					{
						grab_max = range_max - *min_len;
					}
					if (max_len)
					{
						grab_min = range_max - *max_len;
					}
					grab_t = &grab_min_t;
					slide_target_value = &range_min;
				}
				else if (_g.index == Index::Max)
				{
					grab_min = range_min;
					grab_max = upper_bound;
					if (min_len)
					{
						grab_min = range_min + *min_len;
					}
					if (max_len)
					{
						grab_max = range_min + *max_len;
					}
					grab_t = &grab_max_t;
					slide_target_value = &range_max;
				}
				else if (_g.index == Index::Inter)
				{
					ImGuiDataTypeStorage& storage = _g.backup_data[2];
					inter_value = reinterpret_cast<T&>(storage);
					inter_clicked_t = RatioFromValue(inter_value);
					grab_min = lower_bound + (inter_value - range_min);
					grab_max = upper_bound - (range_max - inter_value);
					grab_t = &inter_clicked_t;
					slide_target_value = &inter_value;
				}

			};

			// Sets _grabs_overlapping
			auto calc_mouse_hovered_grab = [&]() -> Index
			{
				Index res = Index::None;
				float grab_size_t = grab_sz / slider_usable_sz;
				float half_grab_size_t = 0.5f * grab_size_t;
				// TODO improve grab Inter when narrow
				float upper_min_grab_t = grab_min_t + half_grab_size_t;
				float lower_max_grab_t = grab_max_t - half_grab_size_t;
				float inter_grab_dist_t = lower_max_grab_t - upper_min_grab_t;
				if (inter_grab_dist_t < grab_size_t)
				{
					float a = grab_min_t - half_grab_size_t;
					float b = grab_max_t + half_grab_size_t;
					upper_min_grab_t = ImLerp(a, b, 0.3333333f);
					lower_max_grab_t = ImLerp(a, b, 0.6666666f);
				}
				if (mouse_t < upper_min_grab_t)
				{
					res = Index::Min;
				}
				else if (mouse_t > lower_max_grab_t)
				{
					res = Index::Max;
				}
				else
				{
					res = Index::Inter;
				}
				return res;
			};

			auto setup_inter_value = [&](float t)
			{
				inter_clicked_t = t;
				inter_value = ValueFromRatio(inter_clicked_t);
				reinterpret_cast<T&>(_g.backup_data[2]) = inter_value;
			};

			if (isActive())
			{
				bool set_new_value = false;
				float clicked_t = 0.0f;
				if (g.ActiveIdSource == ImGuiInputSource_Mouse)
				{
					if (!g.IO.MouseDown[0])
					{
						clearActive();
					}
					else
					{
						// Init index when made active
						if (_g.index == Index::None)
						{
							_g.index = calc_mouse_hovered_grab();
						}
						_hovered_index = _g.index;

						if (g.ActiveIdIsJustActivated && _g.index == Index::Inter)
						{
							setup_inter_value(mouse_t);
						}

						select_target_value();

						if (g.ActiveIdIsJustActivated)
						{
							const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, *grab_t);
							const bool clicked_around_grab = (mouse_abs_pos >= grab_pos - grab_sz * 0.5f - 1.0f) && (mouse_abs_pos <= grab_pos + grab_sz * 0.5f + 1.0f); // No harm being extra generous here.
							g.SliderGrabClickOffset = (clicked_around_grab && is_floating_point) ? mouse_abs_pos - grab_pos : 0.0f;
						}
						if (slider_usable_sz > 0.0f)
							clicked_t = ImSaturate((mouse_abs_pos - g.SliderGrabClickOffset - slider_usable_pos_min) / slider_usable_sz);
						if (axis == ImGuiAxis_Y)
							clicked_t = 1.0f - clicked_t;
						set_new_value = true;
					}
				}
				else if (g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad)
				{
					// TODO
				}

				if (set_new_value)
				{
					if ((g.LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) || (flags & ImGuiSliderFlags_ReadOnly))
					{
						set_new_value = false;
					}
				}

				if (set_new_value)
				{
					T v_new = ValueFromRatio(clicked_t);
					v_new = ImClamp(v_new, grab_min, grab_max);
					// Round to user desired precision based on format string
					if (is_floating_point && !(flags & ImGuiSliderFlags_NoRoundToFormat))
						v_new = RoundScalarWithFormatT<Type32>(format, data_type, v_new);

					// Apply result
					if (*slide_target_value != v_new)
					{
						if (_g.index == Index::Inter)
						{
							T offset = v_new - inter_value;
							range_min += offset;
							range_max = range_min + range_len;
							reinterpret_cast<T&>(_g.backup_data[2]) = v_new;
						}
						else
						{
							*slide_target_value = v_new;
						}
						res.flags |= res.SLIDER_BIT;
						res.flags |= [&]() -> decltype(res.flags)
						{
							switch (_g.index)
							{
								case Index::Min: return res.EDIT_MIN_BIT; break;
								case Index::Inter: return res.EDIT_MIN_BIT | res.EDIT_MAX_BIT; break;
								case Index::Max: return res.EDIT_MAX_BIT; break;
							}
							return 0;
						}();
					}
				}
			}
			else
			{
				if (_hovered)
				{
					_hovered_index = calc_mouse_hovered_grab();
				}
			}

			// Prepare rendering
			{
				_min_grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_min_t);
				_max_grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_max_t);
				_grab_size = grab_sz;
				_grab_ok = true;
			}

			return res;
		}

		InputRangeRes behaviour(void* range, const void* bounds, const void* len_bounds, const char* format, ImGuiSliderFlags flags)
		{
			auto dispatch = [&] <std::arithmetic T> ()
			{
				using Type32 = Type32<T>;
				InputRangeRes res;
				if constexpr (std::same_as<Type32, T>)
				{
					res = this->behaviourT<T>(static_cast<T*>(range), static_cast<const T*>(bounds), static_cast<const T*>(len_bounds), format, flags);
				}
				else
				{
					T* t_range = static_cast<T*>(range);
					const T* t_bounds = static_cast<const T*>(bounds);
					const T* t_len_bounds = static_cast<const T*>(len_bounds);
					Type32 my_range[2] = { static_cast<Type32>(t_range[0]), static_cast<Type32>(t_range[1]) };
					Type32 my_bounds[2] = { static_cast<Type32>(t_bounds[0]), static_cast<Type32>(t_bounds[1]) };
					Type32 my_len_bounds[2];
					if (len_bounds)
					{
						my_len_bounds[0] = static_cast<Type32>(t_len_bounds[0]);
						my_len_bounds[1] = static_cast<Type32>(t_len_bounds[1]);
					}
					res = this->behaviourT<Type32>(my_range, my_bounds, len_bounds ? my_len_bounds : nullptr, format, flags);
					if (res.operator bool())
					{
						t_range[0] = my_range[0];
						t_range[1] = my_range[1];
					}
				}
				return res;
			};

			InputRangeRes res = {};
			switch (_data_type)
			{
			case ImGuiDataType_S8: res = dispatch.template operator() < ImS8 > (); break;
			case ImGuiDataType_U8: res = dispatch.template operator() < ImU8 > (); break;
			case ImGuiDataType_S16: res = dispatch.template operator() < ImS16 > (); break;
			case ImGuiDataType_U16: res = dispatch.template operator() < ImU16 > (); break;
			case ImGuiDataType_S32: res = dispatch.template operator() < ImS32 > (); break;
			case ImGuiDataType_U32: res = dispatch.template operator() < ImU32 > (); break;
			case ImGuiDataType_S64: res = dispatch.template operator() < ImS64 > (); break;
			case ImGuiDataType_U64: res = dispatch.template operator() < ImU64 > (); break;
			case ImGuiDataType_Float: res = dispatch.template operator() < float > (); break;
			case ImGuiDataType_Double: res = dispatch.template operator() < double > (); break;
			}
			return res;
		}

		void renderSliderGrabs(const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
		{
			assert(_grab_ok);
			ImGuiContext& g = *GImGui;
			const ImGuiStyle& style = g.Style;
			ImGuiWindow* window = GetCurrentWindow();;
			const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
		
			auto render_grab = [&](ImRect const& bb, ImGuiCol color_id, float alpha = 1)
			{
				window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(color_id, alpha), style.GrabRounding, ImDrawFlags_None);
			};

			const float max_font_size = g.FontSize;
			const size_t value_buf_len = 128;
			char value_buf[value_buf_len];

			struct GrabLabel
			{
				char* txt;
				uint len;
				float size;
			};

			auto render_grab_txt =  [&](ImVec2 const& pos, GrabLabel const& label)
			{
				RenderText(pos, label.txt, label.txt + label.len);
			};

			auto calc_grab_label_txt = [&](GrabLabel& gl, const void* p_data)
			{
				gl.len = DataTypeFormatString(gl.txt, value_buf_len / 2, _data_type, p_data, format);
				gl.size = CalcTextSize(gl.txt, gl.txt + gl.len).x; // UnHandled case: if the text is rendered vertically (independent of the slider verticality option!)
			};

			const bool bounds_equal = DataTypeCompare(_data_type, p_min, p_max) == 0;
			bool merge_grabs = bounds_equal;

			GrabLabel min_label{.txt = value_buf}, max_label{.txt = value_buf + value_buf_len / 2};
			calc_grab_label_txt(min_label, p_min);

			ImRect min_grab_bb, max_grab_bb;
			const float h_grab_size = 0.5f * _grab_size;
			if (axis == ImGuiAxis_X)
			{
				min_grab_bb = ImRect(_min_grab_pos - h_grab_size, _frame.Min.y + _grab_padding, _min_grab_pos + h_grab_size, _frame.Max.y - _grab_padding);
				max_grab_bb = ImRect(_max_grab_pos - h_grab_size, _frame.Min.y + _grab_padding, _max_grab_pos + h_grab_size, _frame.Max.y - _grab_padding);
			}
			else
			{
				min_grab_bb = ImRect(_frame.Min.x + _grab_padding, _min_grab_pos - h_grab_size, _frame.Max.x - _grab_padding, _min_grab_pos + h_grab_size);
				max_grab_bb = ImRect(_frame.Min.x + _grab_padding, _max_grab_pos - h_grab_size, _frame.Max.x - _grab_padding, _max_grab_pos + h_grab_size);
			}


			ImRect inter_bb_wide = ImRect(min_grab_bb.Min, max_grab_bb.Max);
			ImRect inter_bb_reduced = ImRect(min_grab_bb.Max, max_grab_bb.Min);
			const bool grabs_overlapping = (_max_grab_pos - h_grab_size) - (_min_grab_pos + h_grab_size) < _grab_size;
			
			auto calc_overlapping_highligh = [&](Index index)
			{
				float t = static_cast<float>(index) / 3.0f;
				float p = (static_cast<float>(index) + 1.0f) / 3.0f;
				const ImRect bb = inter_bb_wide;
				ImRect hovered_bb = bb;
				hovered_bb.Min[axis] = ImLerp(bb.Min[axis], bb.Max[axis], t);
				hovered_bb.Max[axis] = ImLerp(bb.Min[axis], bb.Max[axis], p);
				return hovered_bb;
			};

			const bool should_render_highligh = isActive() || _hovered;
			auto render_highlight = [&](ImRect const& rect, Index index = Index::None)
			{
				float rounding = 0;
				float t = g.Time;
				const float freq = 1;
				float alpha = std::lerp(0.2, 0.4, std::sqr(sin(t * freq)));
				window->DrawList->AddRectFilled(rect.Min, rect.Max, GetColorU32(ImVec4(1, 1, 1, alpha)), rounding, ImDrawFlags_None);
			};


			if (merge_grabs)
			{
				render_grab(min_grab_bb, ImGuiCol_SliderGrab);
				
				if (should_render_highligh)
				{
					render_highlight(calc_overlapping_highligh(_hovered_index));
				}
				if (g.LogEnabled)
					LogSetNextTextDecoration("{", "}");
				ImRect label_bb = CenterRect(min_grab_bb, ImVec2(min_label.size, g.FontSize));
				label_bb = FitRectInside(_frame, label_bb, true);
				render_grab_txt(label_bb.Min, min_label);
			}
			else
			{
				calc_grab_label_txt(max_label, p_max);
				render_grab(inter_bb_wide, _g.index == Index::Inter ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab, 0.5f);
				render_grab(min_grab_bb, _g.index == Index::Min ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
				render_grab(max_grab_bb, _g.index == Index::Max ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);

				if (should_render_highligh)
				{
					ImRect hovered_bb;
					if (grabs_overlapping)
					{
						hovered_bb = calc_overlapping_highligh(_hovered_index);
					}
					else
					{
						switch (_hovered_index)
						{
						case Index::Min: hovered_bb = min_grab_bb; break;
						case Index::Inter: hovered_bb = inter_bb_reduced; break;
						case Index::Max: hovered_bb = max_grab_bb; break;
						}
					}
					render_highlight(hovered_bb);
				}

				ImRect label_min_bb = CenterRect(min_grab_bb, ImVec2(min_label.size, g.FontSize));
				ImRect label_max_bb = CenterRect(max_grab_bb, ImVec2(max_label.size, g.FontSize));
				// Position min and max labels to avoid overlapping while remaining in the frame
				// There is no perfect solution for some edge cases when there is not enough space to fit both labels
				{
					ImRect frame = _frame;
					float padding = _grab_padding;
					if (frame.GetSize()[axis] > 4 * padding)
					{
						ImVec2 padding2(0, 0);
						padding2[axis] = padding;
						frame.Expand(-padding2);
					}
					ImRect min_constraint = frame, max_constraint = frame;
					label_min_bb = FitRectInside(min_constraint, label_min_bb, true);
					label_max_bb = FitRectInside(max_constraint, label_max_bb, true);

					const float min_inter_label_space = g.FontSize;
					float text_overlap = (label_max_bb.Min[axis] - min_inter_label_space - label_min_bb.Max[axis]);
					const float text_overlap_epsilon = 1e-2;
					if (text_overlap < -text_overlap_epsilon)
					{
						const float lower_margin = ImMax(0.0f, label_min_bb.Min[axis] - min_constraint.Min[axis]);
						const float upper_margin = ImMax(0.0f, max_constraint.Max[axis] - label_max_bb.Max[axis]);
						const float minimum_margin = ImMin(lower_margin, upper_margin);
						const float maximum_margin = ImMax(lower_margin, upper_margin);
						float lower_d = 0, upper_d = 0;

						float* minimum_d = lower_margin < upper_margin ? &lower_d : &upper_d;
						float* maximum_d = lower_margin < upper_margin ? &upper_d : &lower_d;

						float half_overlap = -text_overlap / 2;
						*minimum_d = ImMin(half_overlap, minimum_margin);
						float remainding = (-text_overlap) - *minimum_d;
						*maximum_d = ImMin(remainding, maximum_margin);
						
						ImVec2 d(0, 0);
						d[axis] = lower_d;
						label_min_bb.Translate(-d);
						d[axis] = upper_d;
						label_max_bb.Translate(d);
					}
				}

				if (g.LogEnabled)
					LogSetNextTextDecoration("{", "");
				render_grab_txt(label_min_bb.Min, min_label);

				if (g.LogEnabled)
					LogSetNextTextDecoration(": ", "}");
				render_grab_txt(label_max_bb.Min, max_label);
			}
		}

		void renderCommon(const char* label, ImGuiSliderFlags flags)
		{
			ImGuiContext& g = *GImGui;
			const ImGuiStyle& style = g.Style;
			RenderNavCursor(_frame, _id);
			const ImU32 frame_col = GetColorU32(isActive() ? ImGuiCol_FrameBgActive : _hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
			RenderFrame(_frame.Min, _frame.Max, frame_col, true, style.FrameRounding);
			const ImVec2 label_size = CalcTextSize(label, NULL, true);
			if (label_size.x > 0.0f)
			{
				RenderText(ImVec2(_frame.Max.x + style.ItemInnerSpacing.x, _frame.Min.y + style.FramePadding.y), label);
			}
		}

		// min:max
		// min;len
		// len!max
		InputRangeRes declareSliderInput(const char* label, void* range_min, void* range_max, const void* p_bounds_min, const void* p_bounds_max, bool clamp_bounds, const char* format, ImGuiSliderFlags flags)
		{
			InputRangeRes res = InputRangeExImpl(
				label,
				_frame,
				_id,
				_data_type,
				_data_type_info->Size,
				range_min, range_max,
				p_bounds_min, p_bounds_max,
				format,
				!clamp_bounds,
				ImGuiInputTextFlags_None
			);
			return res;
		}

		InputRangeRes declare(const char* label, ImGuiDataType data_type, void* range, const void* bounds, const void* p_len_bounds, const char* format, ImGuiSliderFlags flags)
		{
			ImGuiWindow* window = GetCurrentWindow();
			if (window->SkipItems)
				return {};
			ImGuiContext& g = *GImGui;
			const ImGuiStyle& style = g.Style;

			_id = window->GetID(label);
			_data_type = data_type;
			const float w = CalcItemWidth();

			const ImVec2 label_size = CalcTextSize(label, NULL, true);
			const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
			const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
			_frame = frame_bb;

			_data_type_info = DataTypeGetInfo(data_type);
			void* p_min = range;
			void* p_max = static_cast<char*>(range) + _data_type_info->Size;

			if (format == NULL)
				format = _data_type_info->PrintFmt;

			_temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
			ItemSize(total_bb, style.FramePadding.y);
			if (!ItemAdd(total_bb, _id, &frame_bb, _temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
				return {};

			_hovered = ItemHoverable(frame_bb, _id, g.LastItemData.ItemFlags);
			_temp_input_active = _temp_input_allowed && TempInputIsActive(_id);

			InputRangeRes res = {};
			if (!_temp_input_active)
			{
				const bool clicked = _hovered && IsMouseClicked(slide_click_button, ImGuiInputFlags_None, _id);
				const bool double_clicked = (_hovered && g.IO.MouseClickedCount[0] == 2 && TestKeyOwner(ImGuiKey_MouseLeft, _id));
				const bool make_active = (clicked || double_clicked || g.NavActivateId == _id);

				if (make_active && (clicked || double_clicked))
				{
					SetKeyOwner(ImGuiKey_MouseLeft, _id);
				}
				if (make_active && _temp_input_allowed)
				{
					if ((clicked && g.IO.KeyCtrl) || double_clicked || (g.NavActivateId == _id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
					{
						_temp_input_active = true;
					}
				}
				if (g.IO.ConfigDragClickToInputText && _temp_input_allowed && !_temp_input_active)
				{
					if (g.ActiveId == _id && _hovered && g.IO.MouseReleased[0] && !IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * DRAG_MOUSE_THRESHOLD_FACTOR))
					{
						g.NavActivateId = _id;
						g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
						_temp_input_active = true;
					}
				}

				if (clicked)
				{
					// Will be determined by behaviour
					_g.index = Index::None;
				}
				// Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
				if (make_active)
				{
					std::memcpy(&_g.backup_data[0], &p_min, _data_type_info->Size);
					std::memcpy(&_g.backup_data[1], &p_max, _data_type_info->Size);
				}

				if (make_active && !_temp_input_active)
				{
					SetActiveID(_id, window);
					SetFocusID(_id, window);
					FocusWindow(window);
					g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
				}
			}

			if (_temp_input_active)
			{	
				const void* bounds_min = nullptr;
				const void* bounds_max = nullptr;
				{
					bounds_min	= bounds;
					bounds_max = static_cast<const char*>(bounds) + _data_type_info->Size;
				}
				res = declareSliderInput(label, p_min, p_max, bounds_min, bounds_max, (flags & ImGuiSliderFlags_ClampOnInput), format, flags);
			}
			else
			{
				res = behaviour(range, bounds, p_len_bounds, format, flags);
				renderCommon(label, flags);
				renderSliderGrabs(p_min, p_max, format, flags);
			}
			
			IMGUI_TEST_ENGINE_ITEM_INFO(_id, label, g.LastItemData.StatusFlags | (_temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));

			return res;
		}

		static InputRangeRes Declare(const char* label, ImGuiDataType data_type, void* range, const void* bounds, const void* p_len_bounds, const char* format, ImGuiSliderFlags flags)
		{
			SliderRangeWidgetImpl impl;
			return impl.declare(label, data_type, range, bounds, p_len_bounds, format, flags);
		}
	};

	SliderRangeWidgetImpl::Globals SliderRangeWidgetImpl::_g = {};

	InputRangeRes SliderRangeEx(const char* label, ImGuiDataType data_type, void* range, const void* bounds, const void* p_len_bounds, const char* format, ImGuiSliderFlags flags)
	{
		return SliderRangeWidgetImpl::Declare(label, data_type, range, bounds, p_len_bounds, format, flags);
	}


	void RenderTextFitEx(ImRect const& rect, const char* label, const char* label_end, float max_font_size, ImVec2 align)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImDrawList* draw_list = window->DrawList;
		if (label_end == nullptr)
		{
			label_end = FindRenderedTextEnd(label);
		}

		ImFont* font = g.Font;
		float font_size = g.FontSize;
		const char* remaining = nullptr;
		ImVec2 text_size = font->CalcTextSizeA(font_size, rect.GetWidth(), 0, label, label_end, &remaining);

		ImVec2 pos = rect.GetTL();
		if (align.x > 0.0f) pos.x = ImMax(pos.x, pos.x + (rect.Max.x - pos.x - text_size.x) * align.x);
		if (align.y > 0.0f) pos.y = ImMax(pos.y, pos.y + (rect.Max.y - pos.y - text_size.y) * align.y);

		if (label != label_end)
		{
			draw_list->AddText(font, font_size, pos, GetColorU32(ImGuiCol_Text), label, label_end);
			if (g.LogEnabled)
			{
				LogRenderedText(&pos, label, label_end);
			}
		}
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

	bool InspectRange(Context& ctx, const char* label, Range32i* range, Range32i bounds, bool allow_remaining)
	{
		assert(range);
		bool res = false;
		bool has_remaining = allow_remaining && range->len == range->NPos;
		if (has_remaining)
		{
			res |= ImGui::SliderInt(label, &range->begin, bounds.begin, bounds.end() - 1, nullptr, ImGuiSliderFlags_None);
		}
		else
		{
			int range_edit[2] = { range->begin, range->end() - 1 };
			int bounds_[2] = { bounds.begin, bounds.end() - 1 };
			float v_speed = float(bounds.len) / (ImGui::CalcItemWidth() * 0.5f);
			//res |= ImGui::DragIntRange2(label, range_edit, range_edit + 1, v_speed, bounds.begin, bounds.end() - 1, nullptr, nullptr, ImGuiSliderFlags_None);
			const ImGui::InputRangeRes input_res = ImGui::SliderRangeEx(label, ImGuiDataType_S32, range_edit, bounds_, nullptr, nullptr, ImGuiSliderFlags_None);
			res |= static_cast<bool>(input_res);
			if (res)
			{
				range->begin = range_edit[0];
				range->len = range_edit[1] - range_edit[0] + 1;
				if (input_res.flags & input_res.INPUT_TEXT_BIT)
				{
					if (input_res.separator == ':')
					{
						if (allow_remaining && input_res.max_tag == ' ')
						{
							range->len = -1;
						}
					}
				}
			}
		}
		if (allow_remaining)
		{
			ImGui::SameLine();
			if (ImGui::Checkbox("Remaining", &has_remaining))
			{
				if (has_remaining)
				{
					range->len = range->NPos;
				}
				else
				{
					range->len = bounds.end() - range->begin + 1;
				}
				res |= true;
			}
		}
		return res;
	}
}