#pragma once

#include <vkl/GUI/Panel.hpp>

#include <concepts>
#include <bitset>

namespace vkl::GUI
{
	class PanelHolder : public Panel
	{
	public:

		enum DockCommand : uchar
		{
			None, // Will do nothing (default imgui behaviour)
			ToThis, // Will dock as a new tab next to this
			Default = ToThis,
			ToID, // Dock to a specified id (in DockParams)
			_BaseDir, // Split in a dir from this
			Left = _BaseDir + ImGuiDir_Left, 
			Right = _BaseDir + ImGuiDir_Right,
			Up = _BaseDir + ImGuiDir_Up,
			Down = _BaseDir + ImGuiDir_Down,
			_CommandMask = std::bitMask<typename std::underlying_type<DockCommand>::type>(std::bit_width(Down)),
		};

		struct DockParams
		{
			union
			{
				ImGuiID dock_id = {};
				float split_ratio; // in [-1, 1], 0 is default (means 50%-50%)
			};
		};

	protected:

		struct Child
		{
			std::shared_ptr<Panel> panel = {};
			bool should_focus = true;
			bool declare = true;
			DockCommand dock_command = {};
			DockParams dock_params = {};
		};

		std::unordered_map<Id, Child> _childs;
		MyVector<Id> _declare_ids;
		bool _childs_ids_valid = false;
		bool _disable_from_holder_ctx_stack = false;
		uchar _dock_has_split = {};
		std::array<ImGuiID, 4> _dock_split = {};

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		PanelHolder(CreateInfo const& ci);

		PanelHolder(VkApplication* app, std::string const& name = {});

		ImGuiID getDockSplitID(ImGuiDir dir, float ratio);

	public:

		virtual ~PanelHolder() override;

		void declarePanelsMenu(Context& ctx);

		virtual void declareMenu(Context& ctx) override;

		virtual void declare(Context& ctx, bool keep_open=false) override;

		virtual std::shared_ptr<Panel> getChild(Id id) const;

		// set panel to nullptr to close the child
		virtual void setChild(Id id, std::shared_ptr<Panel> const& panel = nullptr, DockCommand dock_command = DockCommand::Default, DockParams dock_param = {});

		template <std::invocable<> CreateFn, std::derived_from<Panel> _Panel = typename std::remove_cvref_t<std::invoke_result_t<CreateFn>>::element_type>
		std::shared_ptr<_Panel> getOrCreateChild(Id id, CreateFn const& create_fn)
		{
			std::shared_ptr<_Panel> res;
			auto it = _childs.find(id);
			if (it != _childs.end())
			{
				res = std::dynamic_pointer_cast<_Panel>(it->second.panel);
			}
			else
			{
				res = create_fn();
				_childs[id].panel = res;
				_childs_ids_valid = false;
			}
			return res;
		}

		template <std::invocable<> CreateFn, std::derived_from<Panel> _Panel = typename std::remove_cvref_t<std::invoke_result_t<CreateFn>>::element_type>
		std::shared_ptr<_Panel> openChild(Id id, CreateFn const& create_fn, DockCommand dock_command = DockCommand::Default, DockParams dock_param = {})
		{
			std::shared_ptr<_Panel> child = getOrCreateChild(id, create_fn);
			setChild(id, child, dock_command, dock_param);
			return child;
		}

		virtual void closeAllChilds();

		void setDisableFromHolderCtxStack(bool disable = true)
		{
			_disable_from_holder_ctx_stack = disable;
		}
	};

	THAT_DECLARE_ENUM_CLASS_OPERATORS(PanelHolder::DockCommand);
}