#include <vkl/GUI/MainPanel.hpp>

#include <imgui/imgui.h>

namespace vkl::GUI
{
	MainPanel::MainPanel(VkApplication* app, std::string name) :
		PanelHolder(PanelHolder::CI{ .app = app, .name = name })
	{
		_show_menu = true;
		_can_close = false;
	}

	MainPanel::~MainPanel()
	{

	}

	void MainPanel::declareMenu(Context& ctx)
	{
		for (uint i = 0; i < _menus.size32(); ++i)
		{
			PanelMenu& menu = _menus[i];
			if (ImGui::BeginMenu(menu.name.c_str()))
			{
				if (menu.pre_menu)
				{
					menu.pre_menu(ctx, this);
					ImGui::Separator();
				}
				for (uint j = 0; j < menu.options.size32(); ++j)
				{
					PanelMenuOption & option = menu.options[j];
					Id id = option.id ? option.id : reinterpret_cast<Id>(option.panel.get());
					const char* label = (option.label.empty() ? option.panel->name().c_str() : option.label.c_str());
					bool selected = !!option.panel && getChild(id) == option.panel;
					bool enable = !!option.panel && !option.disable;
					if (ImGui::MenuItem(label, nullptr, selected, enable))
					{
						option.panel->setOpen();
						setChild(id, option.panel);
					}
				}
				if (menu.post_menu)
				{
					ImGui::Separator();
					menu.post_menu(ctx, this);
				}
				ImGui::EndMenu();
			}
		}
	}

	void MainPanel::declareInline(Context& ctx)
	{
		bool sep = false;
		if (_inline_declaration)
		{
			_inline_declaration(ctx, this);
			sep = true;
		}

		if (sep)
		{
			ImGui::Separator();
			sep = false;
		}
		for (uint i = 0; i < _inline_panels.size32(); ++i)
		{
			_inline_panels[i].declareInline(ctx);
		}
	}

	void MainPanel::addMenu(PanelMenu const& menu)
	{
		_menus.push_back(menu);
	}

	void MainPanel::addInlinePanel(InlinePanel const& ip)
	{
		_inline_panels.push_back(ip);
		InlinePanel& b = _inline_panels.back();
		if (b.panel)
		{
			if (b.id == 0)
			{
				b.id = reinterpret_cast<Id>(b.panel.get());
			}
		}
		b.type = InlinePanel::Type::CollapseHeader;
	}
}