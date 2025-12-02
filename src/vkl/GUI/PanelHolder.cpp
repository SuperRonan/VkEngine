#include <vkl/GUI/PanelHolder.hpp>
#include <vkl/GUI/Context.hpp>

#include <imgui/imgui.h>

namespace vkl::GUI
{
	PanelHolder::PanelHolder(CreateInfo const& ci) :
		Panel(ci.app, ci.name)
	{
		_show_menu = true;
	}

	PanelHolder::~PanelHolder()
	{

	}

	void PanelHolder::declarePanelsMenu(Context& ctx)
	{
		if (ImGui::BeginMenu("Panels", !_childs.empty()))
		{
			for (auto& [id, child] : _childs)
			{
				if (ImGui::MenuItem(child.panel->name().c_str(), nullptr, nullptr, child.panel->isOpen()))
				{
					child.should_focus = true;
				}
			}
			ImGui::EndMenu();
		}
	}

	void PanelHolder::declareMenu(Context& ctx)
	{
		declarePanelsMenu(ctx);
	}

	void PanelHolder::declare(Context& ctx)
	{
		ctx.pushPanelHolder(this);

		Panel::declare(ctx);

		// Check _declare_ids
		if (!_childs_ids_valid)
		{
			_declare_ids.clear();
			for (auto& [id, child] : _childs)
			{
				_declare_ids.push_back(id);
			}
			_childs_ids_valid = true;
		}

		// Declare childs
		for (auto id : _declare_ids)
		{
			auto it = _childs.find(id);
			if (it != _childs.end())
			{
				auto & [_, child] = *it;
				if (child.declare)
				{
					if (child.should_focus)
					{
						ImGui::SetNextWindowFocus();
						child.should_focus = false;
					}
					child.panel->declare(ctx);
					if (!child.panel->isOpen())
					{
						setChild(id, nullptr);
					}
				}
			}
			else
			{
				assert(!_childs_ids_valid);
				_childs_ids_valid &= false;
			}
		}

		ctx.popPanelHolder();
	}

	std::shared_ptr<Panel> PanelHolder::getChild(Id id) const
	{
		if (_childs.contains(id))
		{
			return _childs.at(id).panel;
		}
		return nullptr;
	}

	void PanelHolder::setChild(Id id, std::shared_ptr<Panel> const& panel)
	{
		if (panel)
		{
			_childs_ids_valid &= _childs.contains(id);
			auto & child = _childs[id];
			child.panel = panel;
			child.should_focus = true;
		}
		else
		{
			_childs.erase(id);
			_childs_ids_valid &= false;
		}
	}
}