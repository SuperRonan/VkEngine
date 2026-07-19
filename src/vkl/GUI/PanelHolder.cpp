#include <vkl/GUI/PanelHolder.hpp>
#include <vkl/GUI/Context.hpp>

#include <imgui/imgui_internal.h>

namespace vkl::GUI
{

	static constexpr ImGuiDir ExtractSplitDir(PanelHolder::DockCommand dc)
	{
		return static_cast<ImGuiDir>((dc & PanelHolder::DockCommand::_SplitMask) >> PanelHolder::DockCommand::_SplitBitOffset);
	}

	static constexpr ImGuiDir GetSplitDir(PanelHolder::DockCommand dc)
	{
		if (dc & PanelHolder::DockCommand::HasSplit)
		{
			return ExtractSplitDir(dc);
		}
		return ImGuiDir_None;
	}


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

	ImGuiID PanelHolder::getDockSplitID(Context& ctx, ImGuiDir dir, float ratio)
	{
		const bool fix_imgui_bug = true;
		getOrCreateDockId(ctx);
		assert(dir >= ImGuiDir_Left);
		assert(dir <= ImGuiDir_Down);
		ImGuiID &res = _dock_split[dir];
		const uchar bit = uchar(1) << static_cast<uchar>(dir);
		if (!(bit & _dock_has_split))
		{
			bool _bug_previous_dock_is_active = false;
			ImGuiWindow* _bug_window_to_restore = nullptr;
			if (fix_imgui_bug)
			{
				ImGuiDockNode* node = ImGui::DockBuilderGetNode(getDockId());
				if (node->Windows.Size > 0)
				{
					_bug_window_to_restore = node->Windows[0];
					_bug_previous_dock_is_active = _bug_window_to_restore->DockIsActive;
				}
			}
			res = ImGui::DockBuilderSplitNode(_dock_id, dir, ratio, nullptr, nullptr);
			ImGui::DockBuilderFinish(_dock_id);
			
			if (fix_imgui_bug && _bug_window_to_restore)
			{
				_bug_window_to_restore->DockIsActive |= _bug_previous_dock_is_active;
			}
			_dock_has_split |= bit;
		}
		return res;
	}

	void PanelHolder::declarePanelsMenu(Context& ctx)
	{
		if (ImGui::BeginMenu("Panels", !_children.empty()))
		{
			if (ImGui::MenuItem("Close all"))
			{
				closeAllChildren();
			}
			ImGui::Separator();

			for (auto& [id, child] : _children)
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

	struct CompatiblePanel
	{
		Panel* ptr = nullptr;
		u64 compatibility = {};
		size_t focus_time = 0;
		uint dist = 0;

		CompatiblePanel() = default;

		CompatiblePanel(Context& ctx, PanelHolder::Child const& child, Panel* p, uint dist)
		{
			if (child.panel && child.panel->isOpen())
			{
				ptr = child.panel.get();
				compatibility = child.panel->getPanelCompatibility(ctx, p);
				focus_time = child.latest_focus_time;
				this->dist = dist;
			}
		}

		std::strong_ordering compareWith(Context& ctx, CompatiblePanel const& rhs)
		{
			if (!ptr || !rhs.ptr)
			{
				return ptr <=> rhs.ptr;
			}
			std::strong_ordering res = compatibility <=> rhs.compatibility;
			if (res == std::strong_ordering::equal)
			{
				res = rhs.dist <=> dist; // reversed, closer is better
				if(res == std::strong_ordering::equal)
				{
					res = focus_time <=> rhs.focus_time;
				}
			}
			return res;
		}

		bool isGoodEnough(Context& ctx, uint current_dist)
		{
			return false;
		}
	};

	static CompatiblePanel _FindCompatiblePanelImpl(Context& ctx, PanelHolder* h, Panel* p, Panel* prev, uint dist)
	{
		const uint down_dist = 1;
		const uint up_dist = 1;
		CompatiblePanel res = {};
		// Check direct children
		for (auto& [id, child] : h->children())
		{
			if (child.panel && child.panel.get() != p)
			{
				CompatiblePanel cp(ctx, child, p, dist);
				if (cp.compareWith(ctx, res) > 0)
				{
					res = cp;
				}
			}
		}
		if(res.isGoodEnough(ctx, dist))
		{
			return res;
		}
		// Check recursive children
		for (auto& [id, child] : h->children())
		{
			if(child.panel && child.panel.get() != prev)
			{
				if(PanelHolder* ch = dynamic_cast<PanelHolder*>(child.panel.get()))
				{
					CompatiblePanel cp = _FindCompatiblePanelImpl(ctx, ch, p, h, dist + down_dist);
					if (cp.compareWith(ctx, res) > 0)
					{
						res = cp;
					}
				}
			}
		}
		if (res.isGoodEnough(ctx, dist))
		{
			return res;
		}
		// Check recursive parent;
		{
			auto stack = ctx.getPanelHolderStack();
			auto found = std::ranges::find(stack, h);
			if (found != stack.end())
			{
				size_t index = found - stack.begin();
				if (index > 0)
				{
					PanelHolder* parent = stack[index - 1];
					if (parent != prev)
					{
						CompatiblePanel cp = _FindCompatiblePanelImpl(ctx, parent, p, h, dist + up_dist);
						if (cp.compareWith(ctx, res) > 0)
						{
							res = cp;
						}
					}
				}
			}
		}
		return res;
	}

	Panel* PanelHolder::FindCompatiblePanel(Context& ctx, PanelHolder* h, Panel* p)
	{
		return _FindCompatiblePanelImpl(ctx, h, p, nullptr, 0).ptr;
	}

	void PanelHolder::processChildDocking(Context& ctx, Child& child)
	{
		if (child.dock_command == DockCommand::None)
		{
			return;
		}
		ImGuiID dock_id = {};
		if (child.dock_command & DockCommand::SearchCompatible)
		{
			Panel* p = FindCompatiblePanel(ctx, this, child.panel.get());
			if (p)
			{
				dock_id = p->getOrCreateDockId(ctx);
			}
		}

		if (dock_id == InvalidDockID)
		{
			DockCommand dc = child.dock_command & DockCommand::_CommandMask;
			ImGuiDir split_dir = GetSplitDir(child.dock_command);
			if (dc == DockCommand::ToThis)
			{
				dock_id = getOrCreateDockId(ctx);
				if (split_dir != ImGuiDir_None)
				{
					dock_id = getDockSplitID(ctx, split_dir, child.dock_params.split_ratio * 0.5f + 0.5f);
				}
			}
			else if (dc == DockCommand::ToID)
			{
				dock_id = child.dock_params.dock_id;
				if (split_dir != ImGuiDir_None)
				{
					application()->logger()("GUI: Docking to a split ImGuiID not implemented yet!", Logger::Options::TagWarning | Logger::Options::VerbosityImportant);
				}
			}
			else if (dc == DockCommand::ToPtr)
			{
				dock_id = child.dock_params.panel->getOrCreateDockId(ctx);
				if (split_dir != ImGuiDir_None)
				{
					PanelHolder* ph = dynamic_cast<PanelHolder*>(child.dock_params.panel);
					if (ph)
					{
						dock_id = ph->getDockSplitID(ctx, split_dir, child.dock_params.split_ratio * 0.5f + 0.5f);
					}
					else
					{
						application()->logger()("GUI: Docking to a split Panel not implemented yet!", Logger::Options::TagWarning | Logger::Options::VerbosityImportant);
					}
				}
			}
		}
		child.panel->setDockID(dock_id);
		child.dock_command = {};
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
		if (!_children_ids_valid)
		{
			_declare_ids.clear();
			for (auto& [id, child] : _children)
			{
				_declare_ids.push_back(id);
			}
			_children_ids_valid = true;
		}

		// Declare childs
		for (auto id : _declare_ids)
		{
			auto it = _children.find(id);
			if (it != _children.end())
			{
				auto & [_, child] = *it;
				if (child.declare)
				{
					if (child.panel->isOpen())
					{
						processChildDocking(ctx, child);
						if (child.should_focus)
						{
							ImGui::SetNextWindowFocus();
							child.should_focus = false;
						}
						child.panel->declare(ctx);
						if (child.panel->hasFocus())
						{
							child.latest_focus_time = ctx.getFrameIndex();
						}
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
				assert(!_children_ids_valid);
				_children_ids_valid &= false;
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
		if (_children.contains(id))
		{
			return _children.at(id).panel;
		}
		return nullptr;
	}

	void PanelHolder::setChild(Id id, std::shared_ptr<Panel> const& panel, DockCommand dock_command, DockParams dock_param)
	{
		if (panel)
		{
			_children_ids_valid &= _children.contains(id);
			auto & child = _children[id];
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
			_children.erase(id);
			_children_ids_valid &= false;
		}
	}

	void PanelHolder::closeAllChildren()
	{
		_children.clear();
		_children_ids_valid &= false;
	}
}