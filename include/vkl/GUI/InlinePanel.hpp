#pragma once

#include <vkl/GUI/Panel.hpp>

namespace vkl::GUI
{
	extern bool DetachPanelButton(Context& ctx, std::function<std::shared_ptr<Panel> const& (Context& ctx)> const& panel, Panel::Id id);

	struct InlinePanel
	{
		enum class Type
		{
			None = 0,
			Child = 1,
			TreeNode = 2,
			CollapseHeader = 3,
		};
		std::shared_ptr<Panel> panel;
		std::string label = {};
		Panel::Id id = {};
		Type type = Type::None;
		bool invalid_panel = false;

		struct ReturnType
		{
			bool declare_inline = false;
			bool detach = false;
		};

		ReturnType declareInline(GUI::Context& ctx);

		static InlinePanel MakeFromUniqePanel(std::shared_ptr<Panel> const& panel, Type type = Type::None);
	};

	struct IndirectInlinePanel : InlinePanel
	{
		using CreateFn = std::function<std::shared_ptr<Panel>(GUI::Context& ctx)>;
		CreateFn make_panel;

		ReturnType declareInline(GUI::Context& ctx, bool keep_open = false);

		void clear()
		{
			panel.reset();
		}

		void reset(CreateFn const& fn, Panel::Id id)
		{
			clear();
			make_panel = fn;
			this->id = id;
		}

		void reset(CreateFn const& fn)
		{
			reset(fn, id);
		}

		static IndirectInlinePanel MakeIndirectPanel(CreateFn const& fn, std::string_view label, Panel::Id id);

		template <class Target>
		static IndirectInlinePanel MakeUniqueIndirectPanel(std::shared_ptr<Target> const& target)
		{
			auto create_fn = [target](GUI::Context& ctx)
			{
				return target->makeInspector(target, ctx);
			};
			return MakeIndirectPanel(create_fn, target->name(), reinterpret_cast<Panel::Id>(target.get()));
		}

		template <std::strictly_derived_from<AbstractInstanceHolder> Descriptor>
		static IndirectInlinePanel MakeInstanceIndirectPanelFromDesc(std::shared_ptr<Descriptor> const& desc, std::string_view label = "Instance")
		{
			auto create_fn = [desc](GUI::Context& ctx)
			{
				return desc->instance()->makeInspector(desc->instance(), ctx);
			};
			return MakeIndirectPanel(create_fn, label, reinterpret_cast<Panel::Id>(desc.get()));
		}
	};
}