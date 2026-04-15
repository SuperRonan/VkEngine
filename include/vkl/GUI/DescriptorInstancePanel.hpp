#pragma once

#include <vkl/VkObjects/AbstractInstance.hpp>

#include <vkl/GUI/TargetInspector.hpp>
#include <vkl/GUI/TypedInlineInspector.hpp>

namespace vkl::GUI
{
	class AbstractInstancePanel : public TargetInspectorBase
	{
	public:
		AbstractInstancePanel(std::shared_ptr<AbstractInstance> const& target, std::string_view class_type):
			TargetInspectorBase(target, std::format("{} Instance", class_type))
		{ }

		AbstractInstance* target() const
		{
			return static_cast<AbstractInstance*>(_target.get());
		}

		std::shared_ptr<AbstractInstance> const& targetPtr() const
		{
			return std::reinterpret_pointer_downcast<AbstractInstance>(_target);
		}
	};

	namespace impl
	{
		template <class C>
		concept HasClassName = requires
		{
			{ C::ClassName } -> that::concepts::StringLike;
		};

		template <class C>
		std::string_view GetClassNameOf()
		{
			if constexpr (HasClassName<C>)
			{
				return C::ClassName;
			}
			else
			{
				return typeid(C).name();
			}
		}
	}

	template <std::strictly_derived_from<AbstractInstance> Instance>
	class InstanceInspector : public AbstractInstancePanel
	{
	protected:

	public:

		InstanceInspector(std::shared_ptr<Instance> const& target) :
			AbstractInstancePanel(target, impl::GetClassNameOf<Instance>())
		{ }

		Instance* target() const
		{
			return static_cast<Instance*>(_target.get());
		}

		std::shared_ptr<Instance> const& targetPtr() const
		{
			return std::reinterpret_pointer_downcast<Instance>(_target);
		}
	};

	class AbstractDescriptorPanel : public TargetInspector<AbstractInstanceHolder>
	{
	protected:

		using Parent = TargetInspector<AbstractInstanceHolder>;

		ObjectInlineInspector _instance_panel;

	public:
		AbstractDescriptorPanel(std::shared_ptr<AbstractInstanceHolder> const& target, std::string_view class_name):
			Parent(target, std::format("{} Descriptor", class_name)),
			_instance_panel("Instance")
		{
			_instance_panel.setDisableCreation(true);
			_instance_panel.setAcceptNullptr(true);
		}

		AbstractInstanceHolder* target() const
		{
			return static_cast<AbstractInstanceHolder*>(_target.get());
		}

		std::shared_ptr<AbstractInstanceHolder> const& targetPtr() const
		{
			return std::reinterpret_pointer_downcast<AbstractInstanceHolder>(_target);
		}

		virtual void declareInstance(Context& ctx);
	};

	template <std::strictly_derived_from<AbstractInstanceHolder> Descriptor>
	class DescriptorInspector : public AbstractDescriptorPanel
	{
	protected:

		TypedInlineInspector<typename Descriptor::InstanceType> _instance_panel;

	public:

		DescriptorInspector(std::shared_ptr<Descriptor> const& target) :
			AbstractDescriptorPanel(target, impl::GetClassNameOf<Descriptor::InstanceType>())
		{}

		Descriptor* target() const
		{
			return static_cast<Descriptor*>(_target.get());
		}

		std::shared_ptr<Descriptor> const& targetPtr() const
		{
			return std::reinterpret_pointer_downcast<Descriptor>(_target);
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