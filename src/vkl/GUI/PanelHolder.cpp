#include <vkl/GUI/PanelHolder.hpp>
#include <vkl/GUI/Context.hpp>

#include <imgui/imgui_internal.h>

namespace vkl::GUI
{

	//static constexpr ImGuiDir ExtractSplitDir(PanelHolder::DockCommand dc)
	//{
	//	return static_cast<ImGuiDir>((dc & PanelHolder::DockCommand::_SplitMask) >> PanelHolder::DockCommand::_SplitBitOffset);
	//}

	//static constexpr ImGuiDir GetSplitDir(PanelHolder::DockCommand dc)
	//{
	//	if (dc & PanelHolder::DockCommand::HasSplit)
	//	{
	//		return ExtractSplitDir(dc);
	//	}
	//	return ImGuiDir_None;
	//}


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

	ImGuiID PanelHolder::getDockSplitID(ImGuiDir dir, float ratio)
	{
		{
			ImGuiDockNode* node = ImGui::DockBuilderGetNode(getDockId());
		}
		assert(dir >= ImGuiDir_Left);
		assert(dir <= ImGuiDir_Down);
		ImGuiID &res = _dock_split[dir];
		const uchar bit = uchar(1) << static_cast<uchar>(dir);
		if (!(bit & _dock_has_split))
		{
			res = ImGui::DockBuilderSplitNode(_dock_id, dir, ratio, nullptr, nullptr);
			_dock_has_split |= bit;
		}
		return res;
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
					if (child.dock_command != DockCommand::None)
					{
						DockCommand dc = child.dock_command & DockCommand::_CommandMask;
						ImGuiID dock_id = {};
						if (dc == DockCommand::ToThis)
						{
							dock_id = getOrCreateDockId(ctx);
						}
						else if (dc == DockCommand::ToID)
						{
							dock_id = child.dock_params.dock_id;
						}
						else if (dc == DockCommand::ToPtr)
						{
							dock_id = child.dock_params.panel->getOrCreateDockId(ctx);
						}
						//else // if dc == ImGuiDir
						//{
						//	using U = typename std::underlying_type<DockCommand>::type;
						//	const ImGuiDir split_dir = static_cast<ImGuiDir>(dc - _BaseDir);
						//	dock_id = getDockSplitID(split_dir, (child.dock_params.split_ratio + 1.0f) * 0.5f);
						//}
						child.panel->setDockID(dock_id);
						child.dock_command = {};
					}
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

	void PanelHolder::setChild(Id id, std::shared_ptr<Panel> const& panel, DockCommand dock_command, DockParams dock_param)
	{
		if (panel)
		{
			_childs_ids_valid &= _childs.contains(id);
			auto & child = _childs[id];
			child.panel = panel;
			if (panel->isOpen())
			{
				child.should_focus = true;
				child.dock_command = DockCommand::ToID;
				child.dock_params.dock_id = getDockId();
			}
			child.dock_command = dock_command;
			child.dock_params = dock_param;
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