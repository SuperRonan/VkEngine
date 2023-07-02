#pragma once

#include <vector>
#include <string>
#include "imgui.h"

namespace vkl
{
	class ImGuiRadioButtons
	{
	protected:

		size_t _index = 0;
		std::vector<std::string> _buttons = {};

	public:

		ImGuiRadioButtons(std::vector<std::string> const& labels, size_t default_index = 0) :
			_index(default_index),
			_buttons(labels)
		{}

		bool declare()
		{
			size_t active_index = _index;
			for (size_t i = 0; i < _buttons.size(); ++i)
			{
				bool b = ImGui::RadioButton(_buttons[i].c_str(), i == _index);
				if (b)	active_index = i;
				if (i != _buttons.size() - 1)
					ImGui::SameLine();
			}
			bool changed = _index != active_index;
			_index = active_index;
			return changed;
		}

		constexpr size_t index()const
		{
			return _index;
		}

	};
}