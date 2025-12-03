#pragma once

#include <vkl/GUI/PanelHolder.hpp>
#include <vkl/GUI/InlinePanel.hpp>

namespace vkl::GUI
{

	class MainPanel : public PanelHolder
	{
	public:

		struct PanelMenuOption
		{
			std::shared_ptr<Panel> panel;
			std::string label = {}; // empty -> use the panel's name
			Id id = {}; // 0 -> use the panel ptr value
			bool disable = false;
		};

		struct PanelMenu
		{
			std::string name;
			MyVector<PanelMenuOption> options;
			DeclareFunction pre_menu = {}, post_menu = {};
		};

		MyVector<PanelMenu> _menus;

		MyVector<InlinePanel> _inline_panels;

		DeclareFunction _inline_declaration;

	protected:


	public:

		MainPanel(VkApplication* app, std::string name);

		virtual ~MainPanel() override;

		virtual void declareMenu(Context& ctx) override;

		virtual void declareInline(Context& ctx) override;

		void addMenu(PanelMenu const& menu);

		void setInlineDeclaration(DeclareFunction const& declare_fn)
		{
			_inline_declaration = declare_fn;
		}

		void addInlinePanel(InlinePanel const& panel);
	};

}