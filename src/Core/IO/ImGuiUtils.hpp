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

		ImGuiRadioButtons(std::vector<std::string> const& labels) :
			_buttons(labels)
		{}

		size_t declare()
		{
			size_t active_index = _index;
			for (size_t i = 0; i < _buttons.size(); ++i)
			{
				bool b = ImGui::RadioButton(_buttons[i].c_str(), i == _index);
				if (b)	active_index = i;
				if (i != _buttons.size() - 1)
					ImGui::SameLine();
			}
			_index = active_index;
			return _index;
		}

	};
}