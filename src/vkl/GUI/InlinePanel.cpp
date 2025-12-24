
#include <vkl/GUI/InlinePanel.hpp>
#include <vkl/GUI/PanelHolder.hpp>
#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>

namespace vkl::GUI
{
	template <class GetPanelFn = std::nullptr_t>
	bool DetachPanelButtonT(Context& ctx, GetPanelFn const& get_panel_fn, Panel::Id id)
	{
		bool res = false;
		constexpr const bool can_declare = !std::same_as<GetPanelFn, std::nullptr_t>;
		if (ImGui::DetachButton())
		{
			if constexpr (can_declare)
			{
				std::shared_ptr<Panel> const& panel = get_panel_fn(ctx);
				assert(!!panel);
				panel->setOpen();
				auto holder = ctx.getTopPanelHolder();
				holder->setChild(id, panel);
				res = true;
			}
		}
		return res;
	}

	bool DetachPanelButton(Context& ctx, std::function<std::shared_ptr<Panel> const& (Context& ctx)> const& get_panel_fn, Panel::Id id)
	{
		return DetachPanelButtonT(ctx, get_panel_fn, id);
	}

	template <std::convertible_to<std::function<std::shared_ptr<Panel> const&(Context& ctx)>> GetPanelFn = std::nullptr_t>
	inline InlinePanel::ReturnType DeclareInlinePanelEx(
		GUI::Context& ctx,
		InlinePanel::Type type,
		const char* label,
		Panel::Id id,
		GetPanelFn const& get_panel_fn = {}
	) {
		InlinePanel::ReturnType res = {};
		using Type = InlinePanel::Type;
		ImGui::PushID(id);

		constexpr const bool can_declare = !std::same_as<GetPanelFn, std::nullptr_t>;
		res.declare_inline = can_declare;
		if (!can_declare)
		{
			ImGui::BeginDisabled();
		}

		const auto& declare_detach = [&]()
		{
			res.detach = DetachPanelButtonT(ctx, get_panel_fn, id);
			ImGui::SameLine();
		};

		if (type != Type::Child)
		{
			declare_detach();
		}
		
		if (type == Type::None)
		{
			ImGui::SeparatorText(label);
		}
		if (type == Type::CollapseHeader)
		{
			res.declare_inline &= ImGui::CollapsingHeader(label);
		}
		else if (type == Type::TreeNode)
		{
			res.declare_inline &= ImGui::TreeNode(label);
		}
		else if (type == Type::Child)
		{
			res.declare_inline &= ImGui::BeginChild(label, ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_None);
			if (res.declare_inline)
			{
				declare_detach();
				res.declare_inline &= ImGui::CollapsingHeader(label);
			}
		}

		if (res.declare_inline)
		{
			if constexpr (can_declare)
			{
				auto const& panel = get_panel_fn(ctx);
				panel->setUsed();
				panel->declareInline(ctx);
			}
		}

		if (type == Type::Child)
		{
			ImGui::EndChild();
		}
		else if (type == Type::TreeNode)
		{
			// Nothing...
		}
		else
		{
			ImGui::Separator();
		}
		if (!can_declare)
		{
			ImGui::EndDisabled();
		}
		ImGui::PopID();
		return res;
	}

	InlinePanel::ReturnType InlinePanel::declareInline(GUI::Context& ctx)
	{
		const char* label = this->label.empty() && panel ? panel->name().c_str() : this->label.c_str();
		if(panel && !invalid_panel)
		{
			return DeclareInlinePanelEx(
				ctx,
				type,
				label,
				id,
				[&](GUI::Context& ctx) -> std::shared_ptr<Panel> const& {return panel; }
			);
		}
		else
		{
			return DeclareInlinePanelEx(
				ctx,
				type,
				label,
				id
			);
		}
	}

	InlinePanel InlinePanel::MakeFromUniqePanel(std::shared_ptr<Panel> const& panel, Type type)
	{
		return InlinePanel{
			.panel = panel,
			.id = reinterpret_cast<Panel::Id>(panel.get()),
			.type = type,
		};
	}

	IndirectInlinePanel::ReturnType IndirectInlinePanel::declareInline(GUI::Context& ctx, bool keep_open)
	{
		if (!make_panel || invalid_panel)
		{
			return InlinePanel::declareInline(ctx);
		}
		auto get_panel_fn = [&](GUI::Context& ctx) -> std::shared_ptr<Panel> const&
		{
			auto & holder = *ctx.getTopPanelHolder();
			if (!panel)
			{
				panel = holder.getChild(id);
			}
			if(!panel)
			{
				panel = make_panel(ctx);
				panel->setOpen(false);
				holder.setChild(id, panel);
			}
			return panel;
		};
		ReturnType res = DeclareInlinePanelEx(
			ctx,
			type,
			label.c_str(),
			id,
			get_panel_fn
		);
		if (!res.declare_inline && !!panel && !keep_open)
		{
			clear();
		}
		return res;
	}

	IndirectInlinePanel IndirectInlinePanel::MakeIndirectPanel(CreateFn const& fn, std::string_view label, Panel::Id id)
	{
		IndirectInlinePanel res;
		res.make_panel = fn;
		res.label = label;
		res.id = id;
		res.type = Type::CollapseHeader;
		return res;
	}
}