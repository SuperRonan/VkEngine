#include <vkl/Rendering/SceneUserInterface.hpp>

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
		const std::filesystem::path shader_lib = application()->mountingPoints()["ShaderLib"];
		_render_3D_basis = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".Show3DBasis",
			.vertex_input_desc = GraphicsPipeline::VertexInputWithoutVertices(),
			.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			.draw_count = 3,
			.line_raster_mode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
			.sets_layouts = _sets_layouts,
			.color_attachements = {_target},
			.vertex_shader_path = shader_lib / "Rendering/Show3DBasis.glsl",
			.geometry_shader_path = shader_lib / "Rendering/Show3DBasis.glsl",
			.fragment_shader_path = shader_lib / "Rendering/Show3DBasis.glsl",
		});

		_box_mesh = RigidMesh::MakeCube(RigidMesh::CubeMakeInfo{
			.app = application(),
			.name = name() + ".BoxMesh",
			.center = glm::vec3(0.5),
			.wireframe = true,
		});
		
		DefinitionsList render_box_3D_defs = {
			"DIMENSIONS 3",
		};
		_render_3D_box = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".Render3DBox",
			.vertex_input_desc = RigidMesh::vertexInputDescOnlyPos3D(),
			.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
			.line_raster_mode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
			.sets_layouts = _sets_layouts,
			.color_attachements = { _target },
			.depth_stencil = _depth,
			.write_depth = false,
			.depth_compare_op = VK_COMPARE_OP_LESS,
			.vertex_shader_path = shader_lib / "Rendering/Mesh/renderOnlyPos.vert",
			.fragment_shader_path = shader_lib / "Rendering/Mesh/renderUniColor.frag",
			.definitions = std::move(render_box_3D_defs),
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
		bool update_3D_basis = _show_view_basis || _show_world_basis || _gui_selected_node.hasValue() || ctx.updateAnyway();
		if (update_3D_basis)
		{
			ctx.resourcesToUpdateLater() += _render_3D_basis;
		}

		if (_gui_selected_node.hasValue() || ctx.updateAnyway())
		{
			_box_mesh->updateResources(ctx);
			ctx.resourcesToUpdateLater() += _render_3D_box;
		}
	}

	void SceneUserInterface::execute(ExecutionRecorder& recorder, Camera & camera)
	{
		recorder.pushDebugLabel(name(), true);
		
		static thread_local VertexCommand::DrawInfo vertex_draw_info;
		
		vertex_draw_info.clear();
		auto & draw_list = vertex_draw_info;

		if (_show_world_basis)
		{
			const mat4 pc = camera.getWorldToProj();
			draw_list.pushBack(VertexCommand::DrawCallInfo{
				.name = "world",
				.pc_data = &pc,
				.pc_size = sizeof(pc),
				.draw_count = 3,
				.instance_count = 1,
			});
		}
		if (_show_view_basis)
		{
			glm::mat4 view_3D_basis_matrix = camera.getCamToProj() * translateMatrix<4, float>(glm::vec3(0, 0, -0.25))* camera.getWorldRoationMatrix()* scaleMatrix<4, float>(0.03125);
			draw_list.pushBack(VertexCommand::DrawCallInfo{
				.name = "view",
				.pc_data = &view_3D_basis_matrix,
				.pc_size = sizeof(view_3D_basis_matrix),
				.draw_count = 3,
				.instance_count = 1,
			});
		}
		if (_gui_selected_node.hasValue())
		{
			const mat4 pc = camera.getWorldToProj() * glm::mat4(_gui_selected_node.node.matrix);
			draw_list.pushBack(VertexCommand::DrawCallInfo{
				.name = "selected node",
				.pc_data = &pc,
				.pc_size = sizeof(pc),
				.draw_count = 3,
				.instance_count = 1,
			});
		}
		if (draw_list.calls.size())
		{
			vertex_draw_info.draw_type = DrawType::Draw;

			recorder(_render_3D_basis->with(vertex_draw_info));
		}
		draw_list.clear();
		vertex_draw_info.clear();



		if (_gui_selected_node.hasValue() && _box_mesh->isReadyToDraw())
		{
			vertex_draw_info.clear();
			const auto & model = _gui_selected_node.node.node->model();
			if (model)
			{
				const auto & mesh = model->mesh();
				if (mesh)
				{
					static thread_local VertexDrawCallInfo vdcr;
					vdcr.clear();
					_box_mesh->fillVertexDrawCallInfo(vdcr);

					const AABB3f & aabb = mesh->getAABB();
					Mat4 aabb_matrix = translateMatrix<4, float>(aabb.bottom())* scaleMatrix<4, float>(aabb.diagonal());
					
					const std::string name = mesh->name() + "::AABB";
					const Render3DBoxPC pc{
						.matrix = camera.getWorldToProj() * Mat4(_gui_selected_node.node.matrix) * aabb_matrix,
						.color = glm::vec4(1, 1, 1, 1),
					};
					draw_list.pushBack(VertexCommand::DrawCallInfo{
						.name = name,
						.pc_data = &pc,
						.pc_size = sizeof(pc),
						.draw_count = vdcr.draw_count,
						.instance_count = vdcr.instance_count,
						.index_buffer = vdcr.index_buffer,
						.index_type = vdcr.index_type,
						.num_vertex_buffers = vdcr.vertex_buffers.size32(),
						.vertex_buffers = vdcr.vertex_buffers.data(),
					});
					
					vdcr.clear();
				}
			}
		}

		if (draw_list.calls.size())
		{
			vertex_draw_info.draw_type = DrawType::DrawIndexed;
			recorder(_render_3D_box->with(vertex_draw_info));
		}
		draw_list.clear();
		vertex_draw_info.clear();
		
		recorder.popDebugLabel();

		vertex_draw_info.clear();
	}

	void SceneUserInterface::declareGui(GuiContext& ctx)
	{
		ImGui::PushID(name().c_str());
		if (ImGui::CollapsingHeader("Options"))
		{
			ImGui::ColorPicker3("Ambient", &_scene->_ambient.x);
			ImGui::Checkbox("show world 3D basis", &_show_world_basis);
			ImGui::Checkbox("show view 3D basis", &_show_view_basis);

			ImGui::InputInt("Base ShadowMap resolution", (int*)&_scene->_light_resolution);
		}

		if (ImGui::CollapsingHeader("Tree"))
		{
			Scene::DAG::FastNodePath path;
			auto declare_node = [&](std::shared_ptr<Scene::Node> const& node, Mat4 const& matrix, bool is_selected_path_so_far, const auto& recurse) -> void
			{
				Mat4 node_matrix = matrix * node->matrix4x4();
				const std::string & node_gui_name = node->name();
				const bool node_visible = node->visible();

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

				if (!node_visible)
				{
					float c = 0.666;
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(c, c, c, 1));
				}
				const bool node_open = ImGui::TreeNodeEx(node_gui_name.c_str(), flags);
				if (!node_visible)
				{
					ImGui::PopStyleColor(1);
				}

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
				ImVec4 c = ctx.style().invalid_red;
				c.x *= 0.8;
				ImGui::PushStyleColor(ImGuiCol_Button, c);
				c.x *= 1.1;
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c);
				c.x *= 1.1;
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, c);
				bool clicked = ImGui::Button("Close");
				ImGui::PopStyleColor(3);
				if (clicked)
				{
					_gui_selected_node.clear();
				}
				else
				{
					bool visible = node->visible();
					if (ImGui::Checkbox("Visible", &visible))
					{
						node->setVisibility(visible);
					}
					
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