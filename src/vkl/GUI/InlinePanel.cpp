
#include <vkl/GUI/InlinePanel.hpp>
#include <vkl/GUI/PanelHolder.hpp>
#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>

#include <imgui/imgui_internal.h>

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
		const char* child_label,
		Panel::Id id,
		GetPanelFn const& get_panel_fn = {}
	) {
		InlinePanel::ReturnType res = {};
		using Type = InlinePanel::Type;
		ImGui::PushID(id);

		if (label && label[0] == char(0))
		{
			label = nullptr;
		}
		if (child_label && child_label[0] == char(0))
		{
			child_label = nullptr;
		}
		assert(!!label);

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
			const float top_item_width = ImGui::GetCurrentWindow()->DC.ItemWidth;
			res.declare_inline &= ImGui::BeginChild(child_label ? child_label : label, ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
			if (child_label)
			{
				ImGui::SeparatorText(child_label);
			}
			const auto& style = ImGui::GetStyle();
			float item_width = top_item_width - 2 * style.FramePadding.x - style.FrameBorderSize;
			ImGui::PushItemWidth(item_width); // Use the full available witdh for the child
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
			ImGui::PopItemWidth();
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
		if(panel && !invalid_panel)
		{
			return DeclareInlinePanelEx(
				ctx,
				type,
				label.c_str(),
				child_label.c_str(),
				id,
				[&](GUI::Context& ctx) -> std::shared_ptr<Panel> const& {return panel; }
			);
		}
		else
		{
			return DeclareInlinePanelEx(
				ctx,
				type,
				label.c_str(),
				child_label.c_str(),
				id
			);
		}
	}

	InlinePanel InlinePanel::MakeFromUniqePanel(std::shared_ptr<Panel> const& panel, Type type)
	{
		return InlinePanel{
			.panel = panel,
			.label = panel->name(),
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
			child_label.c_str(),
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
		res.type = Type::Child;
		return res;
	}
}