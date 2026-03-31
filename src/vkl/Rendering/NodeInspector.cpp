#include <vkl/Rendering/NodeInspector.hpp>

#include <vkl/Rendering/SceneUserInterface.hpp>

#include <imgui/imgui_internal.h>

#include <vkl/GUI/FancyButtons.hpp>

namespace vkl::GUI
{
	NodeInspector::NodeInspector(std::shared_ptr<Scene::Node> const& node) :
		GUI::Panel(node->application(), node->name()),
		_node(node)
	{
		resetName();
		_model_panel.init("Model");
		_light_panel.init("Light");
	}

	void NodeInspector::setUnique(bool unique)
	{
		if (_unique != unique)
		{
			_unique = unique;
			resetName();
		}
	}

	void NodeInspector::resetName()
	{
		if (_node)
		{
			if (_unique)
			{
				setName("Node Inspector - " + _node->name() + "###NodeInspector");
			}
			else
			{
				setName("Node Inspector - " + _node->name());
			}
		}
		else
		{
			setName("Node Inspector");
		}
	}

	void NodeInspector::declareInline(GUI::Context& ctx)
	{
		if (hasFocus() && _parent)
		{
			_parent->setNodeInFocus(this);
		}

		bool visible = _node->visible();
		if (ImGui::Checkbox("Visible", &visible))
		{
			_node->setVisibility(visible);
		}

		{
			const auto stack_color = ctx.pushStack();
			ImGui::PushStyleColor(ImGuiCol_Border, stack_color);
			ImGui::PushStyleColor(ImGuiCol_Separator, stack_color);

			const float top_item_width = ImGui::GetCurrentWindow()->DC.ItemWidth;
			if (ImGui::BeginChild("Transform", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None))
			{
				ImGui::PopStyleColor(2);
				const auto& style = ImGui::GetStyle();
				float item_width = top_item_width - 2 * style.FramePadding.x - style.FrameBorderSize;
				ImGui::PushItemWidth(item_width); // Use the full available witdh for the child

				if (ImGui::CollapsingHeader("Transform"))
				{
					bool changed = false;

					ImGui::Text("Collapsed Matrix");
					Matrix3x4f node_matrix = _node->matrix3x4();
					ImGui::BeginDisabled();
					ImGui::DragMatrix("", node_matrix);
					ImGui::EndDisabled();

					ImGui::Separator();
					float range = 10;
					ImGuiSliderFlags flags = ImGuiSliderFlags_NoRoundToFormat;
					if (ImGui::Button("Reset"))
					{
						_node->resetAuxiliaryTransform();
					}
					ImGui::SameLine();
					if (ImGui::Button("Collapse Matrix"))
					{
						_node->collapseAuxiliaryTransform();
					}
					auto& scale = _node->scale();
					const bool scale_is_uniform = scale.x() == scale.y() && scale.x() == scale.z();
					if (_uniform_scale_edit && scale_is_uniform)
					{
						if (ImGui::DragFloat("Scale", scale.data(), 0.1, -range, range, "%.3f", flags | ImGuiSliderFlags_Logarithmic))
						{
							scale.setConstant(scale.x());
						}
					}
					else
					{
						ImGui::DragFloat3("Scale", scale.data(), 0.1, -range, range, "%.3f", flags | ImGuiSliderFlags_Logarithmic);
					}
					ImGui::SameLine();
					if (ImGui::SquareButton("1"))
					{
						scale.setConstant(1);
					}
					ImGui::SameLine();
					if (ImGui::InboxCheckbox("Lock", &_uniform_scale_edit))
					{
						_uniform_scale_edit_set = true;
					}

					ImGui::SliderAngle3("Rotation", _node->rotation().data(), -180, 180, "%.2f", flags);
					ImGui::SameLine();
					if (ImGui::SquareButton("0##R"))
					{
						_node->rotation().setConstant(0);
					}

					ImGui::DragFloat3("Translation", _node->translation().data(), 0.1, -range, range, "%.3f", flags | ImGuiSliderFlags_Logarithmic);
					ImGui::SameLine();
					if (ImGui::SquareButton("0##T"))
					{
						_node->translation().setConstant(0);
					}
				}

				ImGui::PopItemWidth();
			}
			ImGui::PushStyleColor(ImGuiCol_Border, stack_color);
			ImGui::PushStyleColor(ImGuiCol_Separator, stack_color);
			ImGui::EndChild();
			ImGui::PopStyleColor(2);
			ctx.popStack();
		}

		_model_panel.declareInline(ctx, _node->model());

		_light_panel.declareInline(ctx, _node->light());

		ImGui::SeparatorText("Children");
		{
			SceneUserInterface::DeclareNodeHierarchy(ctx, _parent, _node);
		}
	}
}