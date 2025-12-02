#pragma once

#include <vkl/GUI/Panel.hpp>

namespace vkl::GUI
{
	namespace concepts
	{
		template <class T>
		concept GUIWrappableType = requires(T & t, Context& ctx)
		{
			t.declareGUI(ctx);
		};
		
		template <class T>
		concept GuiWrappableType = requires(T & t, Context & ctx)
		{
			t.declareGui(ctx);
		};

		template <class T>
		concept PanelWrappableType = GUIWrappableType<T> || GuiWrappableType<T>;
	}

	template <concepts::PanelWrappableType Type>
	class PanelRefWrapper final : public Panel
	{
	protected:

		Type* _ref;

	public:

		PanelRefWrapper(Type* ref, std::string name = {})
			requires std::derived_from<Type, VkObject> :
			Panel(ref->application(), name.empty() ? ref->name() : name),
			_ref(ref)
		{
		
		}

		virtual ~PanelRefWrapper() override
		{

		}

		virtual void declareInline(Context& ctx)
		{
			if constexpr (concepts::GUIWrappableType<Type>)
			{
				_ref->declareGUI(ctx);
			}
			else if constexpr (concepts::GuiWrappableType<Type>)
			{
				_ref->declareGui(ctx);
			}
		}
	};
}