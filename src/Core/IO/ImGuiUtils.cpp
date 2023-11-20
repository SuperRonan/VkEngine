#include "ImGuiUtils.hpp"
#include <cassert>

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
			const bool b = ImGui::RadioButton(_options[i].name.c_str(), i == _index);
			if (!_options[i].desc.empty())
			{
				ImGui::SetItemTooltip(_options[i].desc.c_str());
			}
			if (b)	active_index = i;
			if (same_line && (i != _options.size() - 1))
				ImGui::SameLine();
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
			}
			ImGui::EndCombo();
			res = _index != active_index;
			_index = active_index;
		}
		return res;
	}
}