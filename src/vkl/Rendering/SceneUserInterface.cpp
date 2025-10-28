#include <vkl/Rendering/SceneUserInterface.hpp>

#include <vkl/Rendering/SceneLoader.hpp>

#include <imgui/misc/cpp/imgui_stdlib.h>

#include <vkl/IO/ImGuiUtils.hpp>

#include <ShaderLib/Rendering/Scene/SceneFlags.h>

namespace vkl
{
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
			selected_node.node.matrix = DiagonalMatrix<3, 4>(1.0f);
		}
	}

	void SceneUserInterface::createInternalResources()
	{
		_render_pass = std::make_shared<RenderPass>(RenderPass::SPCI{
			.app = application(),
			.name = name() + ".RenderPass",
			.colors = {
				AttachmentDescription2{
					.flags = AttachmentDescription2::Flags::Blend,
					.format = _target->format(),
					.samples = _target->sampleCount(),
				},
			},
			.depth_stencil = AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::ReadOnly, _depth),
			.read_only_depth = true,
		});

		Framebuffer::CI fb_ci{
			.app = application(),
			.name = name() + ".Framebuffer",
			.render_pass = _render_pass,
			.attachments = {_target},
		};
		if (_depth)
		{
			fb_ci.attachments.push_back(_depth);
		}

		_framebuffer = std::make_shared<Framebuffer>(std::move(fb_ci));

		const std::filesystem::path shader_lib = "ShaderLib:/";
		GraphicsPipeline::LineRasterizationState line_raster_state{
			.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT
		};
		_render_3D_basis = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".Show3DBasis",
			.vertex_input_desc = GraphicsPipeline::VertexInputWithoutVertices(),
			.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			.draw_count = 3,
			.line_raster_state = line_raster_state,
			.sets_layouts = _sets_layouts,
			.extern_render_pass = _render_pass,
			.vertex_shader_path = shader_lib / "Rendering/Show3DBasis.glsl",
			.geometry_shader_path = shader_lib / "Rendering/Show3DBasis.glsl",
			.fragment_shader_path = shader_lib / "Rendering/Show3DBasis.glsl",
		});

		_box_mesh = RigidMesh::MakeCube(RigidMesh::CubeMakeInfo{
			.app = application(),
			.name = name() + ".BoxMesh",
			.center = Vector3f::Constant(0.5),
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
			.line_raster_state = line_raster_state,
			.sets_layouts = _sets_layouts,
			.extern_render_pass = _render_pass,
			.write_depth = false,
			.depth_compare_op = _depth ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_ALWAYS,
			.vertex_shader_path = shader_lib / "Rendering/Geometry/renderOnlyPos.vert",
			.fragment_shader_path = shader_lib / "Rendering/Geometry/renderUniColor.frag",
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

		bool render = false;

		if (update_3D_basis)
		{
			ctx.resourcesToUpdateLater() += _render_3D_basis;
			render = true;
		}

		if (_gui_selected_node.hasValue() || ctx.updateAnyway())
		{
			_box_mesh->updateResources(ctx);
			ctx.resourcesToUpdateLater() += _render_3D_box;
			render = true;
		}

		if (render)
		{
			_render_pass->updateResources(ctx);
			_framebuffer->updateResources(ctx);
		}
	}

	void SceneUserInterface::execute(ExecutionRecorder& recorder, Camera & camera)
	{
		recorder.pushDebugLabel(name(), true);

		bool began_render_pass = false;
		auto begin_render_pass_IFN = [&]()
		{
			if (!began_render_pass && !recorder.getCurrentRenderingStatus())
			{
				recorder.beginRenderPass(RenderPassBeginInfo{
					.render_pass = _render_pass->instance(),
					.framebuffer = _framebuffer->instance(),
				});
				began_render_pass = true;
			}
		};
		
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
			Matrix4f view_3D_basis_matrix = (camera.getCamToProj() * Matrix4f(TranslationMatrix(Vector3f(0, 0, -0.25)))).eval() * (Matrix4f(camera.getWorldRoationMatrix()) * Matrix4f(DiagonalMatrix<3>(0.03125f))).eval();
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
			const mat4 pc = camera.getWorldToProj() * Matrix4f(_gui_selected_node.node.matrix);
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
			begin_render_pass_IFN();
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
					Mat4 aabb_matrix = Mat4(TranslationMatrix(aabb.bottom())) * Mat4(DiagonalMatrixV(aabb.diagonal()));
					
					const std::string name = mesh->name() + "::AABB";
					Mat4 pc_matrix = ((camera.getWorldToProj() * Mat4(_gui_selected_node.node.matrix)).eval() * aabb_matrix);
					const Render3DBoxPC pc{
						.matrix = pc_matrix,
						.color = vec4(1, 1, 1, 1),
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
			begin_render_pass_IFN();
			vertex_draw_info.draw_type = DrawType::DrawIndexed;
			recorder(_render_3D_box->with(vertex_draw_info));
		}
		draw_list.clear();
		vertex_draw_info.clear();
		
		if (began_render_pass)
		{
			recorder.endRenderPass();
		}

		recorder.popDebugLabel();

		vertex_draw_info.clear();
	}

	bool SceneUserInterface::CreateNodePopUp::canCreateNodeFromFile() const
	{
		return std::filesystem::exists(_path) && std::filesystem::is_regular_file(_path);
	}

	void SceneUserInterface::CreateNodePopUp::open(std::shared_ptr<Scene::Node> const& parent)
	{
		_parent = parent;
		ImGui::OpenPopup(name().data(), flags());
	}

	void SceneUserInterface::CreateNodePopUp::close()
	{
		ImGui::CloseCurrentPopup();
	}

	std::array<SDL_DialogFileFilter, 1> Wavefront_file_filter = {
		SDL_DialogFileFilter{
			.name = "Wavefront OBJ",
			.pattern = "obj",
		}
	};

	void SceneUserInterface::CreateNodePopUp::openFileDialog()
	{
		SDL_DialogFileCallback cb = [](void* p_user_data, const char* const* file_list, int filter)
		{
			auto* that = static_cast<CreateNodePopUp*>(p_user_data);

			if (file_list && *file_list && filter == 0)
			{
				const char* file_path = *file_list;
				std::unique_lock lock(that->_file_dialog_mutex);
				that->_path = file_path;
				that->_path_str = that->_path.string();
			}
			that->_file_dialog_open = false;
		};

		SDL_Window* parent_sdl_window = static_cast<SDL_Window*>(ImGui::GetWindowViewport()->PlatformHandle);
		
		SDL_ShowOpenFileDialog(cb, this, parent_sdl_window, Wavefront_file_filter.data(), static_cast<int>(Wavefront_file_filter.size()), nullptr, false);
	}

	static ImGuiListSelection mesh_type_selection = ImGuiListSelection::CI{
		.name = "Mesh Type",
		.mode = ImGuiListSelection::Mode::Dropdown,
		.options = {
#define REGISTER_OPTION(Name) \
			ImGuiListSelection::Option{ \
				.name = #Name, \
			},
ITERATE_OVER_RIGID_MESH_MAKE_TYPE(REGISTER_OPTION)
#undef REGISTER_OPTION
		},
	};

	static ImGuiListSelection light_type_selection = ImGuiListSelection::CI{
		.name = "Light Type",
		.mode = ImGuiListSelection::Mode::Dropdown,
		.options = {
			ImGuiListSelection::Option{
				.name = "Point",
			},
			ImGuiListSelection::Option{
				.name = "Directional",
			},
			ImGuiListSelection::Option{
				.name = "Spot",
			},
			ImGuiListSelection::Option{
				.name = "Beam",
			},
		},
	};

	int SceneUserInterface::CreateNodePopUp::declareGUI(GuiContext& ctx)
	{
		int res = 0;
		if (ImGui::BeginPopupModal(name().data(), nullptr, _flags))
		{
			std::unique_lock lock(_file_dialog_mutex, std::defer_lock);
			if (_file_dialog_open)
			{
				lock.lock();
			}

			bool can_create = false;
			const char* create_label = "Create";

			if (ImGui::BeginTabBar("Node type", ImGuiTabBarFlags_None))
			{
				
				if (ImGui::BeginTabItem("Empty Node"))
				{
					_type = 0;
					if (ImGui::InputText("Name", &_path_str))
					{
						
					}
					can_create = true;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Load model file"))
				{
					_type = 1;
					if (ImGui::InputText("Path", &_path_str))
					{
						_path = _path_str;
					}
					ImGui::SameLine();
					ImGui::BeginDisabled(_file_dialog_open);
					if (ImGui::Button("..."))
					{
						openFileDialog();
					}
					ImGui::EndDisabled();
					ImGui::Checkbox("Synchronous load", &_synch);

					if (_path.empty())
					{
						create_label = "Create Empty Node";
					}
					else
					{
						create_label = "Create Node from file";
						if (!canCreateNodeFromFile())
						{
							can_create = false;
						}
						else
						{
							can_create = true;
						}
					}
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Prebuilt model"))
				{
					_type = 2;

					ImGui::InputText("Name", &_path_str);
					size_t index = std::min<size_t>(_sub_type, mesh_type_selection.options().size() - 1);
					if (mesh_type_selection.declare(index))
					{
						_sub_type = static_cast<uint>(index);
					}
					ImGui::SliderFloat("Scale", & _float_3, 1e-2, 1e2, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
					if (static_cast<RigidMesh::RigidMeshMakeInfo::Type>(_sub_type) == RigidMesh::RigidMeshMakeInfo::Type::Sphere ||
						static_cast<RigidMesh::RigidMeshMakeInfo::Type>(_sub_type) == RigidMesh::RigidMeshMakeInfo::Type::Cylinder)
					{
						ImGui::InputInt2("Subdivisions", (int*)_subdivisions.data());
					}
					bool & _material_dielectric = _bool_1;
					ImGui::Checkbox("Dielectric material", &_material_dielectric);
					ImGui::ColorEdit3(_material_dielectric ? "Absorption" : "Albedo", _color.data(), ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					float max_f1 = _material_dielectric ? 2 : 1;
					ImGui::SliderFloat(_material_dielectric ? "Index of Refraction" : "Metallic", &_float_1, 0, max_f1, "%.3f");
					if(!_material_dielectric)
					{
						ImGui::SliderFloat("Roughness", &_float_2, 0, 1, "%.3f", ImGuiSliderFlags_Logarithmic);
					}
					else
					{
						ImGui::Checkbox("Sample Spectral", &_bool_2);
					}


					can_create = true;
					create_label = "Create Model";
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Light"))
				{
					_type = 3;

					ImGui::InputText("Name", &_path_str);

					size_t index = std::min<size_t>(_sub_type - 1, light_type_selection.options().size() - 1);
					if (light_type_selection.declare(index))
					{
						_sub_type = static_cast<uint>(index + 1);
					}

					ImGui::ColorEdit3("Emission", _color.data(), ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					ImGui::Checkbox("Use Shadow Map", &_bool_1);

					can_create = true;
					create_label = "Create Light",

					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			
			ImGui::Separator();
			ImGui::BeginDisabled(!can_create);
			if (ImGui::Button(create_label))
			{
				close();
				res = 1;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				close();
				res = -1;
			}

			ImGui::EndPopup();
		}
		else
		{
			//close();
			res = -1;
		}

		return res;
	}

	std::shared_ptr<Scene::Node> SceneUserInterface::CreateNodePopUp::createNode(VkApplication * app)
	{
		std::shared_ptr<Scene::Node> res;
		if (_type == 0)
		{
			res = std::make_shared<Scene::Node>(Scene::Node::CI{
				.name = _path_str.empty() ? "Empty Node" : _path_str,
				.matrix = Matrix3x4f::Identity(),
			});
		}
		else if(_type == 1)
		{
			if (canCreateNodeFromFile())
			{
				res = std::make_shared<NodeFromFile>(NodeFromFile::CI{
					.app = app,
					.name = _path.filename().string(),
					.matrix = Matrix3x4f::Identity(),
					.path = _path,
					.synch = _synch,
				});
			}
		}
		else if (_type == 2)
		{
			
			BasicModelNodeCreateInfo ci{
				.app = app,
				.name = _path_str.empty() ? "Model" : _path_str,
				.mesh_type = static_cast<RigidMesh::RigidMeshMakeInfo::Type>(_sub_type),
				.subdivisions = uvec4(_subdivisions.x(), _subdivisions.y(), 1, 1),
				.albedo = _color,
				.roughness = _float_2,
				.metallic_or_eta = _float_1,
				.is_dielectric = _bool_1,
				.sample_spectral = _bool_2,
			};
			float scale = _float_3;
			if (scale != 0.0f)
			{
				ci.xform = ScalingMatrix(Vector3f::Constant(scale).eval());
			}
			res = MakeModelNode(ci);
		}
		else if (_type == 3)
		{
			res = MakeLightNode(LightNodeCreateInfo{
				.app = app,
				.name = _path_str.empty() ? "Light" : _path_str,
				.type = static_cast<LightType>(_sub_type),
				.emission = _color,
				.enable_shadow_map = _bool_1,
			});
		}
		return res;
	}

	void SceneUserInterface::declareGui(GuiContext& ctx)
	{
		ImGui::PushID(this);
		if (ImGui::CollapsingHeader("Options"))
		{
			ImGui::ColorEdit3("Ambient", _scene->_ambient.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
			ImGui::ColorEdit3("Sky", _scene->_uniform_sky.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
			
			ImGui::SliderFloat("Sky brightness", &_scene->_uniform_sky_brightness, 0, 12, "%.3f", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_Logarithmic);

			ImGui::SeparatorText("Sun");
			ImGui::SliderAngle("Inclination", &_scene->_solar_disk_direction[0], 0, 180);
			ImGui::SliderAngle("Azimuth", &_scene->_solar_disk_direction[1], -180, 180, "%.1f deg", ImGuiSliderFlags_WrapAround);
			ImGui::SliderAngle("Solar Disk angle", &_scene->_solar_disk_angle, 0, 90, "%.1f deg");

			Light::DeclareEmission(_scene->_solar_disk_emission, _scene->_solar_disk_emission_options);

			ImGui::SeparatorText("Bounds");
			vec3 center = _scene->_aabb.center();
			ImGui::Text("Center: (%f, %f, %f)", center.x(), center.y(), center.z());
			ImGui::Text("Radius: %f", _scene->_radius);
			
			ImGui::Checkbox("show world 3D basis", &_show_world_basis);
			ImGui::Checkbox("show view 3D basis", &_show_view_basis);


			int shadow_resolution = static_cast<int>(_scene->_light_resolution);
			if (ImGui::InputInt("Base ShadowMap resolution", &shadow_resolution, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue & 0))
			{
				_scene->_light_resolution = static_cast<uint32_t>(shadow_resolution);
			}
		}

		auto declare_create_node_popup = [&]()
		{
			int popup_res = _create_node_popup.declareGUI(ctx);
			if (popup_res > 0)
			{
				std::shared_ptr<Scene::Node> new_node = _create_node_popup.createNode(application());
				if (new_node)
				{
					std::shared_ptr<Scene::Node> parent = _create_node_popup.getParent();
					parent->addChild(std::move(new_node));
				}
				_create_node_popup.resetParent();
			}
		};

		if (ImGui::CollapsingHeader("Tree"))
		{
			if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight))
			{
				bool open_create_window = false;
				if (ImGui::MenuItem("New"))
				{
					open_create_window = true;
				}
				ImGui::EndMenu();


				if (open_create_window)
				{
					_create_node_popup.open(_scene->getRootNode());
				}
			}

			declare_create_node_popup();

			Scene::DAG::FastNodePath path;
			auto declare_node = [&](std::shared_ptr<Scene::Node> const& node, Mat3x4 const& matrix, bool is_selected_path_so_far, u32 parent_flags, const auto& recurse) -> void
			{
				ImGui::PushID(node.get());
				u32 node_flags = parent_flags;
				if (!node->visible())
				{
					node_flags &= u32(~0x1);
				}
				Mat3x4 node_matrix = matrix * node->matrix3x4();
				const std::string & node_gui_name = node->name();
				const bool node_visible = (node_flags & 0x1) != 0;

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
				}
				ImGui::PushID("On Node");
				if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
				{
					bool open_create_window = false;
					if (ImGui::MenuItem("New"))
					{
						open_create_window = true;
					}

					ImGui::BeginDisabled();
					if (ImGui::MenuItem("Remove"))
					{
						// TODO
					}
					ImGui::EndDisabled();
					
					ImGui::EndMenu();
					//ImGui::EndPopup();

					if (open_create_window)
					{
						_create_node_popup.open(node);
					}
				}
				declare_create_node_popup();
				ImGui::PopID();

				if (node_open)
				{
					path.path.push_back(0);
					for (size_t i = 0; i < node->children().size(); ++i)
					{
						path.path.back() = i;
						recurse(node->children()[i], node_matrix, is_selected_path_so_far, node_flags, recurse);
					}
					path.path.pop_back();

					ImGui::TreePop();
				}
				ImGui::PopID();
			};

			Mat3x4 root_matrix = Mat3x4::Identity();
			declare_node(_scene->getRootNode(), root_matrix, true, 1, declare_node);
		} // Tree

		bool inspect_node = ImGui::Begin("Node Inspector");
		if (inspect_node)
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
						
						ImGui::Text("Collapsed Matrix");
						Matrix3x4f node_matrix = node->matrix3x4();
						ImGui::BeginDisabled();
						ImGui::DragMatrix("", node_matrix);
						ImGui::EndDisabled();

						ImGui::Separator();
						float range = 10;
						ImGuiSliderFlags flags = ImGuiSliderFlags_NoRoundToFormat;
						if (ImGui::Button("Reset"))
						{
							node->resetAuxiliaryTransform();
						}
						ImGui::SameLine();
						if (ImGui::Button("Collapse Matrix"))
						{
							 node->collapseAuxiliaryTransform();
						}
						ImGui::DragFloat3("Scale", node->scale().data(), 0.1, -range, range, "%.3f", flags | ImGuiSliderFlags_Logarithmic);
						ImGui::SliderAngle3("Rotation", node->rotation().data(), -180, 180, "%.2f", flags);
						ImGui::DragFloat3("Translation", node->translation().data(), 0.1, -range, range, "%.3f", flags | ImGuiSliderFlags_Logarithmic);
					}

					if (!!node->model() && ImGui::CollapsingHeader("Model"))
					{
						node->model()->declareGui(ctx);
					}
					else if (!!node->light() && ImGui::CollapsingHeader("Light"))
					{
						node->light()->declareGui(ctx);
					}
				}


				ImGui::PopID();
			}
		}
		ImGui::End();

		ImGui::PopID();
	}
}