#pragma once

#include <vkl/VkObjects/AbstractInstance.hpp>

#include <vkl/GUI/InlinePanel.hpp>

namespace vkl::GUI
{
	class AbstractInstancePanel : public Panel
	{
	public:
		AbstractInstancePanel(VkObject* target):
			Panel(target->application(), std::format("{} - Instance##{}", target->name(), reinterpret_cast<uintptr_t>(target)))
		{ }
	};

	template <std::strictly_derived_from<AbstractInstance> Instance>
	class InstanceInspector : public AbstractInstancePanel
	{
	protected:

		std::shared_ptr<Instance> _target;

	public:

		InstanceInspector(std::shared_ptr<Instance> const& target) :
			AbstractInstancePanel(target.get()),
			_target(target)
		{ }
	};

	class AbstractDescriptorPanel : public Panel
	{
	public:
		AbstractDescriptorPanel(VkObject* target):
			Panel(target->application(), std::format("{} - Descriptor##{}", target->name(), reinterpret_cast<uintptr_t>(target)))
		{ }
	};

	template <std::strictly_derived_from<AbstractInstanceHolder> Descriptor>
	class DescriptorInspector : public AbstractDescriptorPanel
	{
	protected:

		std::shared_ptr<Descriptor> _target;

		IndirectInlinePanel _instance_panel;

	public:

		DescriptorInspector(std::shared_ptr<Descriptor> const& target) :
			AbstractDescriptorPanel(target.get()),
			_target(target)
		{
			_instance_panel = IndirectInlinePanel::MakeInstanceIndirectPanelFromDesc(_target);
			_instance_panel.type = InlinePanel::Type::Child;
		}

		void declareInstance(Context& ctx)
		{
			_instance_panel.invalid_panel = !_target->instance();
			_instance_panel.id = reinterpret_cast<GUI::Panel::Id>(_target->instance().get());
			_instance_panel.declareInline(ctx);
		}
	};

	template <std::strictly_derived_from<Panel> PanelType, std::strictly_derived_from<VkObject> Target>
	std::shared_ptr<PanelType> MakePanelFromTarget(GUI::Context& ctx, std::shared_ptr<Target> const& target)
	{
		return std::make_shared<PanelType>(target);
	}

	// Compiler bug, can't just namespace concepts
	namespace concepts_
	{
		template <class C>
		concept InspectableType = std::strictly_derived_from<C, VkObject> && std::strictly_derived_from<typename C::InspectorType, Panel>;
	}

	template <concepts_::InspectableType InspectableType>
	std::shared_ptr<typename InspectableType::InspectorType> MakeInspectorFromTarget(GUI::Context& ctx, std::shared_ptr<InspectableType> const& target)
	{
		return MakePanelFromTarget<typename InspectableType::InspectorType>(ctx, target);
	}
}