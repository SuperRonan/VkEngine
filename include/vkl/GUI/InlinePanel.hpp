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
		std::string child_label = {};
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

	struct IndirectInlinePanel : public InlinePanel
	{
		using CreateFn = std::function<std::shared_ptr<Panel>(GUI::Context& ctx)>;
		CreateFn make_panel = {};

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
		static IndirectInlinePanel MakeUniqueIndirectPanel(std::shared_ptr<Target> target, std::string_view label)
		{
			CreateFn create_fn = nullptr;
			if (target)
			{
				create_fn = [target](GUI::Context& ctx)
				{
					return target->makeInspector(target, ctx);
				};
			}
			return MakeIndirectPanel(create_fn, label, reinterpret_cast<Panel::Id>(target.get()));
		}

		template <class Target>
		static IndirectInlinePanel MakeUniqueIndirectPanel(std::shared_ptr<Target> const& target)
		{
			return MakeUniqueIndirectPanel(target, target->name());
		}

		template <std::strictly_derived_from<AbstractInstanceHolder> Descriptor>
		static IndirectInlinePanel MakeInstanceIndirectPanelFromDesc(std::shared_ptr<Descriptor> desc)
		{
			CreateFn create_fn = nullptr;
			if (desc)
			{
				create_fn = [desc](GUI::Context& ctx)
				{
					return desc->instance()->makeInspector(desc->instance(), ctx);
				};
			}
			IndirectInlinePanel res = MakeIndirectPanel(create_fn, "Instance", reinterpret_cast<Panel::Id>(desc.get()));
			res.child_label = "Instance";
			return res;
		}
	};

	template <std::strictly_derived_from<VkObject> Target>
	struct TargetIndirectInlinePanel : public IndirectInlinePanel
	{
		const Target* target = nullptr;

		void init(std::string_view label)
		{
			this->child_label = label;
			this->label = "Empty";
			this->type = Type::Child;
		}

		void setTargetIFN(GUI::Context* ctx, std::shared_ptr<Target> const& target)
		{
			if (this->target != target.get())
			{
				this->target = target.get();
				std::string old_child_label = std::move(this->child_label);
				std::string_view label;
				if (target)
				{
					label = target->name();
					if (label.empty())
					{
						label = "Unnamed";
					}
				}
				else
				{
					label = "Empty";
				}
				*static_cast<IndirectInlinePanel*>(this) = IndirectInlinePanel::MakeUniqueIndirectPanel(target, label);
				this->child_label = std::move(old_child_label);
			}
		}

		ReturnType declareInline(GUI::Context& ctx, std::shared_ptr<Target> const& target, bool keep_open = false)
		{
			setTargetIFN(&ctx, target);
			return IndirectInlinePanel::declareInline(ctx, keep_open);
		}
	};
}