#include <vkl/IO/ImGuiUtils.hpp>
#include <cassert>

#include <vkl/Maths/Transforms.hpp>

#include <numbers>

#include <imgui/imgui_internal.h>

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
}