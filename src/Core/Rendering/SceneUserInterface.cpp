#include "SceneUserInterface.hpp"

namespace vkl
{
	void SceneUserInterface::SelectedNode::bindMatrices()
	{
		if (hasValue())
		{
			gui_collapsed_matrix.bindMatrix((const Mat4x3*)&node.matrix);
			gui_node_matrix.bindMatrix(&node.node->matrix4x3(), false);
		}
		else
		{
			gui_collapsed_matrix.bindMatrix(nullptr);
			gui_node_matrix.bindMatrix(nullptr);
		}
	}

	void SceneUserInterface::checkSelectedNode(SelectedNode& selected_node)
	{
		Scene::DAG::PositionedNode found = _scene->getTree()->findNode(selected_node.path);
		if (found.node == selected_node.node.node)
		{
			selected_node.node.matrix = found.matrix;
		}
		else
		{
			selected_node.node.node.reset();
			selected_node.node.matrix = Mat4x3(1);
		}
	}

	void SceneUserInterface::createInternalResources()
	{
		const std::filesystem::path shaders = ENGINE_SRC_PATH "/Shaders/Rendering/";
		_render_3D_basis = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".Show3DBasis",
			.vertex_input_desc = Pipeline::VertexInputWithoutVertices(),
			.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			.draw_count = 3,
			.line_raster_mode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
			.sets_layouts = _sets_layouts,
			.color_attachements = {_target},
			.vertex_shader_path = shaders / "Show3DBasis.glsl",
			.geometry_shader_path = shaders / "Show3DBasis.glsl",
			.fragment_shader_path = shaders / "Show3DBasis.glsl",
		});
	}

	SceneUserInterface::SceneUserInterface(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_scene(ci.scene),
		_target(ci.target),
		_depth(ci.depth),
		_sets_layouts(ci.sets_layouts)
	{
		createInternalResources();
	}

	SceneUserInterface::~SceneUserInterface()
	{

	}

	void SceneUserInterface::updateResources(UpdateContext& ctx)
	{
		bool update_3D_basis = _show_view_basis || _show_world_basis || ctx.updateAnyway() || _gui_selected_node.hasValue();
		if (update_3D_basis)
		{
			ctx.resourcesToUpdateLater() += _render_3D_basis;
		}
	}

	void SceneUserInterface::execute(ExecutionRecorder& recorder, Camera & camera)
	{
		recorder.pushDebugLabel(name());
		std::vector<VertexCommand::DrawCallInfo> basis_draw_list;
		using namespace std::containers_operators;
		if (_show_world_basis)
		{
			basis_draw_list += VertexCommand::DrawCallInfo{
				.draw_count = 3,
				.pc = camera.getWorldToProj(),
			};
		}
		if (_show_view_basis)
		{
			glm::mat4 view_3D_basis_matrix = camera.getCamToProj() * translateMatrix<4, float>(glm::vec3(0, 0, -0.25))* camera.getWorldRoationMatrix()* scaleMatrix<4, float>(0.03125);
			basis_draw_list += VertexCommand::DrawCallInfo{
				.draw_count = 3,
				.pc = view_3D_basis_matrix,
			};
		}
		if (_gui_selected_node.hasValue())
		{
			basis_draw_list += VertexCommand::DrawCallInfo{
				.draw_count = 3,
				.pc = camera.getWorldToProj() * glm::mat4(_gui_selected_node.node.matrix),
			};
		}
		if (!basis_draw_list.empty())
		{
			recorder(_render_3D_basis->with(VertexCommand::DrawInfo{
				.draw_type = VertexCommand::DrawType::Draw,
				.draw_list = basis_draw_list,
			}));
		}
		recorder.popDebugLabel();
	}

	void SceneUserInterface::declareGui(GuiContext& ctx)
	{
		ImGui::PushID(name().c_str());
		if (ImGui::CollapsingHeader("Options"))
		{
			ImGui::ColorPicker3("Ambient", &_scene->_ambient.x);
			ImGui::Checkbox("show world 3D basis", &_show_world_basis);
			ImGui::Checkbox("show view 3D basis", &_show_view_basis);
		}

		if (ImGui::CollapsingHeader("Tree"))
		{
			Scene::DAG::NodePath path;
			auto declare_node = [&](std::shared_ptr<Scene::Node> const& node, Mat4 const& matrix, bool is_selected_path_so_far, const auto& recurse) -> void
				{
					Mat4 node_matrix = matrix * node->matrix4x4();
					std::string node_gui_name = node->name();

					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
					if (!path.path.empty())
					{
						flags |= ImGuiTreeNodeFlags_OpenOnArrow;
					}
					const bool is_leaf = node->children().empty();
					if (is_leaf)
					{
						flags |= ImGuiTreeNodeFlags_Leaf;
					}

					bool is_selected = false;
					if (is_selected_path_so_far)
					{
						if (!path.path.empty())
						{
							if (_gui_selected_node.path.path.size() >= path.path.size())
							{
								if (_gui_selected_node.path.path[path.path.size() - 1] != path.path.back())
								{
									is_selected_path_so_far = false;
								}
								else
								{
									is_selected = (_gui_selected_node.path.path.size() == path.path.size());
								}
							}
							else
							{
								is_selected_path_so_far = false;
							}
						}
					}

					if (is_selected || node == _gui_selected_node.node.node)
					{
						flags |= ImGuiTreeNodeFlags_Selected;
					}

					const bool node_open = ImGui::TreeNodeEx(node_gui_name.c_str(), flags);

					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && !path.path.empty())
					{
						//std::cout << "Clicked " << node->name() << std::endl;
						_gui_selected_node.node = Scene::DAG::PositionedNode{
							.node = node,
							.matrix = node_matrix,
						};
						_gui_selected_node.path = path;
						_gui_selected_node.bindMatrices();
					}

					if (node_open)
					{
						path.path.push_back(0);
						for (size_t i = 0; i < node->children().size(); ++i)
						{
							path.path.back() = i;
							recurse(node->children()[i], node_matrix, is_selected_path_so_far, recurse);
						}
						path.path.pop_back();

						ImGui::TreePop();
					}
				};

			Mat4 root_matrix = Mat4(1);
			declare_node(_scene->getRootNode(), root_matrix, true, declare_node);
		} // Tree


		if (ImGui::Begin("Node Inspector"))
		{
			if (_gui_selected_node.hasValue())
			{
				checkSelectedNode(_gui_selected_node);
			}
			if (_gui_selected_node.hasValue())
			{
				std::shared_ptr node = _gui_selected_node.node.node;
				ImGui::PushID("Node Inspector");
				ImGui::Text(node->name().c_str());

				ImGui::SameLine();
				if (ImGui::Button("Close"))
				{
					_gui_selected_node.clear();
				}
				else
				{
					if (ImGui::CollapsingHeader("Transform"))
					{
						bool changed = false;
						ImGui::PushID(1);
						ImGui::Text("Collapsed Matrix");
						_gui_selected_node.gui_collapsed_matrix.declare();
						ImGui::PopID();

						ImGui::Separator();

						ImGui::PushID(2);
						ImGui::Text("Node Matrix");
						changed = _gui_selected_node.gui_node_matrix.declare();
						ImGui::PopID();
					}

					if (ImGui::CollapsingHeader("Model"))
					{
						if (node->model())
						{
							node->model()->declareGui(ctx);
						}
						else
						{
							ImGui::Text("None");
							//ImGui::BeginDragDropTarget();

							//ImGui::EndDragDropTarget();
						}
					}

					if (ImGui::CollapsingHeader("Light"))
					{
						if (node->light())
						{
							node->light()->declareGui(ctx);
						}
						else
						{
							ImGui::Text("None");
						}
					}
				}


				ImGui::PopID();
			}
			ImGui::End();
		}

		ImGui::PopID();
	}
}