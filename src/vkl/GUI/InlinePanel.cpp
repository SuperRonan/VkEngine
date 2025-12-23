
#include <vkl/GUI/InlinePanel.hpp>
#include <vkl/GUI/PanelHolder.hpp>
#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>

namespace vkl::GUI
{
	bool DetachPanelButton(Context& ctx, std::shared_ptr<Panel> const& panel, Panel::Id id)
	{
		bool res = false;
		if (ImGui::DetachButton())
		{
			panel->setOpen();
			auto holder = ctx.getTopPanelHolder();
			holder->setChild(id, panel);
			res = true;
		}
		return res;
	}

	void InlinePanel::declareInline(GUI::Context& ctx)
	{
		const char* label = this->label.empty() && panel ? panel->name().c_str() : this->label.c_str();
		const bool can_declare = !!panel;
		InlinePanel::ReturnType res = {};
		ImGui::PushID(id);
		res.declare_inline = can_declare;
		if (!can_declare)
		{
			ImGui::BeginDisabled();
		}
		{
		res.detach = DetachPanelButton(ctx, panel, id);

		ImGui::SameLine();
		if (type == Type::None)
		{
			ImGui::SeparatorText(label);
		}
		if (type == Type::CollapseHeader)
		{
			res.declare_inline &= ImGui::CollapsingHeader(label);
		}
		else if (type == Type::Child)
		{
			res.declare_inline &= ImGui::BeginChild(label);
		}

		if (res.declare_inline)
		{
			panel->declareInline(ctx);
		}

		if (type == Type::Child)
		{
			ImGui::EndChild();
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

	InlinePanel InlinePanel::MakeFromUniqePanel(std::shared_ptr<Panel> const& panel, Type type)
	{
		return InlinePanel{
			.panel = panel,
			.id = reinterpret_cast<Panel::Id>(panel.get()),
			.type = type,
		};
	}
}