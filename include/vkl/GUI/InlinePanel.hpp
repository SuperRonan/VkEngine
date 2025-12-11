#pragma once

#include <vkl/GUI/Panel.hpp>

namespace vkl::GUI
{
	extern bool DetachPanelButton(Context& ctx, std::shared_ptr<Panel> const& panel, Panel::Id id);

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

		static InlinePanel MakeFromUniqePanel(std::shared_ptr<Panel> const& panel, Type type = Type::None);
	};
}