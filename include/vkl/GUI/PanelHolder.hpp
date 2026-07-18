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
			ToPtr, // Do to a specified Panel ptr (will create dock space ID IFN)
			ToID, // Dock to a specified id (in DockParams) (make sure dock space is created)
			_MaxCommand = ToID,
			_CommandBits = std::bit_width(_MaxCommand),
			_CommandMask = std::bitMask<typename std::underlying_type<DockCommand>::type>(_CommandBits),
			HasSplit = 1 << _CommandBits,
			_SplitBitOffset = _CommandBits + 1,
			SplitLeft = HasSplit | (ImGuiDir_Left << _SplitBitOffset),
			SplitRight = HasSplit | (ImGuiDir_Right << _SplitBitOffset),
			SplitUp = HasSplit | (ImGuiDir_Up << _SplitBitOffset),
			SplitDown = HasSplit | (ImGuiDir_Down << _SplitBitOffset),
			_SplitMask = std::bitMask<typename std::underlying_type<DockCommand>::type>(2) << _SplitBitOffset,
			Default = ToThis,
		};

		struct DockParams
		{
			Panel* panel = nullptr;
			ImGuiID dock_id = 0;
			float split_ratio = 0; // in [-1, 1], 0 is default (means 50%-50%)
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

		std::unordered_map<Id, Child> _children;
		MyVector<Id> _declare_ids;
		bool _children_ids_valid = false;
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

		ImGuiID getDockSplitID(Context& ctx, ImGuiDir dir, float ratio);

		void processChildDocking(Context& ctx, Child& child);

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
			auto it = _children.find(id);
			if (it != _children.end())
			{
				res = std::dynamic_pointer_cast<_Panel>(it->second.panel);
			}
			else
			{
				res = create_fn();
				_children[id].panel = res;
				_children_ids_valid = false;
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

		virtual void closeAllChildren();

		void setDisableFromHolderCtxStack(bool disable = true)
		{
			_disable_from_holder_ctx_stack = disable;
		}
	};

	THAT_DECLARE_ENUM_CLASS_OPERATORS(PanelHolder::DockCommand);
}