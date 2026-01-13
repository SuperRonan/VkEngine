#include <vkl/Rendering/NodeInspector.hpp>

#include <vkl/Rendering/SceneUserInterface.hpp>

namespace vkl::GUI
{
	NodeInspector::NodeInspector(SceneUserInterface* parent) :
		GUI::Panel(parent->application(), "")
	{

	}

	NodeInspector::NodeInspector(std::shared_ptr<Scene::Node> const& node, SceneUserInterface* parent) :
		GUI::Panel(parent->application(), node->name()),
		_node(node),
		_parent(parent)
	{

	}

	void NodeInspector::reset(std::shared_ptr<Scene::Node> const& node)
	{
		if (_node != node)
		{
			_node = node;
			resetName();
		}
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
			ImGui::DragFloat3("Scale", _node->scale().data(), 0.1, -range, range, "%.3f", flags | ImGuiSliderFlags_Logarithmic);
			ImGui::SliderAngle3("Rotation", _node->rotation().data(), -180, 180, "%.2f", flags);
			ImGui::DragFloat3("Translation", _node->translation().data(), 0.1, -range, range, "%.3f", flags | ImGuiSliderFlags_Logarithmic);
		}

		if (!!_node->model() && ImGui::CollapsingHeader("Model"))
		{
			_node->model()->declareGui(ctx);
		}
		else if (!!_node->light() && ImGui::CollapsingHeader("Light"))
		{
			_node->light()->declareGui(ctx);
		}

		if (ImGui::CollapsingHeader("Children"))
		{
			for (uint32_t i = 0; i < _node->children().size(); ++i)
			{
				ImGui::PushID(i);
				std::shared_ptr<Scene::Node>const& child = _node->children()[i];
				if (ImGui::SmallButton(child->name().c_str()) && _parent)
				{
					_parent->openNodeInspector(ctx, child);
				}
				ImGui::PopID();
			}
		}
	}
}