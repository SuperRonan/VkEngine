#pragma once

#include <vkl/GUI/Panel.hpp>

#include <concepts>

namespace vkl::GUI
{
	class PanelHolder : public Panel
	{
	protected:

		struct Child
		{
			std::shared_ptr<Panel> panel = {};
			bool should_focus = true;
			bool declare = true;
		};

		std::unordered_map<Id, Child> _childs;
		MyVector<Id> _declare_ids;
		bool _childs_ids_valid = false;

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		PanelHolder(CreateInfo const& ci);

	public:

		virtual ~PanelHolder() override;

		void declarePanelsMenu(Context& ctx);

		virtual void declareMenu(Context& ctx) override;

		virtual void declare(Context& ctx) override;

		virtual std::shared_ptr<Panel> getChild(Id id) const;

		// set panel to nullptr to close the child
		virtual void setChild(Id id, std::shared_ptr<Panel> const& panel = nullptr);

		template <std::derived_from<Panel> _Panel, std::convertible_to<std::function<_Panel(void)>> CreateFn>
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
				_childs[id] = res;
			}
			return res;
		}
	};
}