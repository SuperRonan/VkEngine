#pragma once

#include <vector>
#include <string>
#include "imgui.h"

namespace vkl
{
	class ImGuiListSelection
	{
	public:
		enum class Mode
		{
			RadioButtons,
			Combo,
			Dropdown = Combo,
		};
		
		struct Option
		{
			std::string name;
			std::string desc;
		};

	protected:

		std::string _name = {};
		
		Mode _mode = Mode::Combo;

		size_t _index = 0;
		std::vector<Option> _options = {};
		
		bool _same_line;

	public:

		struct CreateInfo
		{
			std::string name = {};
			Mode mode = Mode::Combo;
			// labels xor options
			std::vector<std::string> labels;
			// labels xor options
			std::vector<Option> options;
			size_t default_index = 0;
			bool same_line = false;
		};
		using CI = CreateInfo;

		ImGuiListSelection() = default;

		ImGuiListSelection(CreateInfo const& ci);

		ImGuiListSelection& operator=(ImGuiListSelection const&) = default;
		ImGuiListSelection& operator=(ImGuiListSelection&&) = default;
		

		bool declareRadioButtons(bool same_line);
		
		bool declareRadioButtons()
		{
			return declareRadioButtons(_same_line);
		}

		bool declareCombo();

		bool declareDropdown()
		{
			return declareCombo();
		}

		bool declare()
		{
			if (_mode == Mode::RadioButtons)
			{
				return declareRadioButtons(_same_line);
			}
			else if (_mode == Mode::Combo)
			{
				return declareCombo();
			}
		}

		constexpr size_t index()const
		{
			return _index;
		}

	};
}