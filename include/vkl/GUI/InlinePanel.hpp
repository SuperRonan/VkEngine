#pragma once

#include <vkl/GUI/Panel.hpp>

namespace vkl::GUI
{
	struct InlinePanel
	{
		enum class Type
		{
			None = 0,
			CollapseHeader = 1,
			Child = 2,
		};
		std::shared_ptr<Panel> panel;
		std::string label = {};
		Panel::Id id = {};
		Type type = Type::None;

		void declareInline(GUI::Context& ctx);
	};
}