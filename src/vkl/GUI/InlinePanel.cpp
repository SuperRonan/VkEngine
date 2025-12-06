
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
		const char* label = this->label.empty() ? panel->name().c_str() : this->label.c_str();
		if (panel)
		{
			ImGui::PushID(panel.get());
		}
		else
		{
			ImGui::PushID(label);
		}

		DetachPanelButton(ctx, panel, id);
		ImGui::SameLine();
		bool declare = !!panel;
		if (type == Type::None)
		{
			ImGui::SeparatorText(label);
		}
		if (type == Type::CollapseHeader)
		{
			declare &= ImGui::CollapsingHeader(label);
		}
		else if (type == Type::Child)
		{
			declare &= ImGui::BeginChild(label);
		}

		if (declare)
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

		ImGui::PopID();
	}

	
}