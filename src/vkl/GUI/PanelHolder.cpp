#include <vkl/GUI/PanelHolder.hpp>
#include <vkl/GUI/Context.hpp>

namespace vkl::GUI
{
	PanelHolder::PanelHolder(CreateInfo const& ci) :
		Panel(ci.app, ci.name)
	{
		windowFlags() |= ImGuiWindowFlags_MenuBar;
	}

	PanelHolder::PanelHolder(VkApplication* app, std::string const& name) :
		PanelHolder(CreateInfo{
			.app = app,
			.name = name,
		})
	{ }

	PanelHolder::~PanelHolder()
	{

	}

	void PanelHolder::declarePanelsMenu(Context& ctx)
	{
		if (ImGui::BeginMenu("Panels", !_childs.empty()))
		{
			if (ImGui::MenuItem("Close all"))
			{
				closeAllChilds();
			}
			ImGui::Separator();

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

	void PanelHolder::declare(Context& ctx, bool keep_open)
	{
		const bool push_to_stack = !_disable_from_holder_ctx_stack;
		if (push_to_stack)
		{
			ctx.pushPanelHolder(this);
		}

		Panel::declare(ctx, true);

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
					if (child.panel->isOpen())
					{
						child.panel->declare(ctx);
					}
					bool keep_child = child.panel->isOpen() || child.panel->isUsed();
					if (keep_child)
					{
						child.panel->setUsed(false);
					}
					else
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

		if (!keep_open)
		{
			ImGui::End();
		}

		if (push_to_stack)
		{
			ctx.popPanelHolder();
		}
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
			if (panel->isOpen())
			{
				child.should_focus = true;
			}
		}
		else
		{
			_childs.erase(id);
			_childs_ids_valid &= false;
		}
	}

	void PanelHolder::closeAllChilds()
	{
		_childs.clear();
		_childs_ids_valid &= false;
	}
}