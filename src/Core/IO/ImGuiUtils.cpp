#include "ImGuiUtils.hpp"
#include <cassert>

#include <Core/Maths/Transforms.hpp>

namespace vkl
{
	ImGuiListSelection::ImGuiListSelection(CreateInfo const& ci) :
		_name(ci.name),
		_mode(ci.mode),
		_index(ci.default_index),
		_same_line(ci.same_line)
	{
		assert(ci.labels.empty() xor ci.options.empty());
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
	}

	bool ImGuiListSelection::declareRadioButtons(bool same_line)
	{
		if (!_name.empty())
		{
			ImGui::Text(_name.c_str());
			if (same_line)
			{
				ImGui::SameLine();
			}
		}
		size_t active_index = _index;
		for (size_t i = 0; i < _options.size(); ++i)
		{
			ImGui::BeginDisabled(_options[i].disable);
			const bool b = ImGui::RadioButton(_options[i].name.c_str(), i == _index);
			if (!_options[i].desc.empty())
			{
				ImGui::SetItemTooltip(_options[i].desc.c_str());
			}
			if (b)	active_index = i;
			if (same_line && (i != _options.size() - 1))
				ImGui::SameLine();
			ImGui::EndDisabled();
		}
		bool changed = _index != active_index;
		if (changed)
		{
			_index = active_index;
		}
		return changed;
	}

	bool ImGuiListSelection::declareCombo()
	{
		bool res = false;
		assert(!_name.empty());
		if (ImGui::BeginCombo(_name.c_str(), _options[_index].name.c_str()))
		{
			size_t active_index = _index;
			for (size_t i = 0; i < _options.size(); ++i)
			{
				ImGui::BeginDisabled(_options[i].disable);
				const bool b = active_index == i;
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
			res = _index != active_index;
			_index = active_index;
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
			Mat3x4 t = glm::transpose(*_matrix);
		
			for (size_t i = 0; i < 3; ++i)
			{
				char row_name[2];
				row_name[0] = 'x' + i;
				row_name[1] = 0;
				changed |= ImGui::DragFloat4(row_name, &t[i].x, 0.1);
			}

			if (changed)
			{
				*_matrix = glm::transpose(t);
			}
		}
		else
		{
			glm::vec3 & t = (*_matrix)[3];
			changed |= ImGui::DragFloat3("Translation", &t.x, 0.1);

			bool changed2 = false;

			glm::vec3 scale(
				glm::length((*_matrix)[0]),
				glm::length((*_matrix)[1]),
				glm::length((*_matrix)[2])
			);
			glm::mat3 rotation = glm::mat3(*_matrix);
			rotation[0] /= scale[0];
			rotation[1] /= scale[1];
			rotation[2] /= scale[2];
			glm::vec3 angles;

			

			changed2 |= ImGui::DragFloat3("Scale", &scale.x, 0.1);

			if (changed2)
			{
				(*_matrix) = translateMatrix<4, float>(t) * scaleMatrix<4, float>(scale) * glm::mat4(rotation);
			}

			changed |= changed2;
		}


		ImGui::EndDisabled();
		return changed;

	}
}