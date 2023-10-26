#include "Renderer.hpp"

namespace vkl
{
	SimpleRenderer::SimpleRenderer(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_exec(ci.exec),
		_sets_layouts(ci.sets_layouts),
		_scene(ci.scene),
		_target(ci.target)
	{
		createInternalResources();
	}

	void SimpleRenderer::createInternalResources()
	{
		_depth = std::make_shared<ImageView>(ImageView::CI{
			.app = application(),
			.name = name() + ".depth",
			.image_ci = Image::CI{
				.app = application(),
				.name = name() + ".depthImage",
				.type = _target->image()->type(),
				.format = VK_FORMAT_D32_SFLOAT,
				.extent = _target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			},
			.range = VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			});
		_exec.declare(_depth);


		_ubo_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".ubo",
			.size = sizeof(UBO),
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
		_exec.declare(_ubo_buffer);


		const uint32_t model_set = application()->descriptorBindingGlobalOptions().set_bindings[size_t(DescriptorSetName::object)].set;
		_model_types = {
			Model::MakeType(Mesh::Type::Rigid, Material::Type::PhysicallyBased),
		};
		std::map<uint32_t, std::shared_ptr<DescriptorSetLayout>> model_layout;
		for (uint32_t model_type : _model_types)
		{
			model_layout[model_type] = Model::setLayout(application(), Model::SetLayoutOptions{ .type = model_type, .bind_mesh = true, .bind_material = true, });
		}

		const std::filesystem::path shaders = PROJECT_SRC_PATH;


		for (uint32_t model_type : _model_types)
		{
			_direct_pipeline._render_scene_direct[model_type] = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RenderSceneDirect",
				.vertex_input_desc = RigidMesh::vertexInputDescFullVertex(), // TODO model type dependent
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.sets_layouts = (_sets_layouts + std::pair{model_set, model_layout[model_type]}),
				.bindings = {
					Binding{
						.buffer = _ubo_buffer,
						.binding = 0,
					},
				},
				.color_attachements = {_target},
				.depth_buffer = _depth,
				.write_depth = true,
				.vertex_shader_path = shaders / "render.vert",
				.fragment_shader_path = shaders / "render.frag",
				.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
				.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
			});
			_exec.declare(_direct_pipeline._render_scene_direct[model_type]);
		}
		{
			_deferred_pipeline._albedo = std::make_shared<ImageView>(ImageView::CI{
				.app = application(),
				.name = name() + ".GBuffer.albedo",
				.image_ci = Image::CI{
					.app = application(),
					.name = name() + ".GBuffer.albedo",
					.type = _target->image()->type(),
					.format = VK_FORMAT_R32G32B32A32_SFLOAT,
					.extent = _target->image()->extent(),
					.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
					.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				},
			});
			_exec.declare(_deferred_pipeline._albedo);

			_deferred_pipeline._position = std::make_shared<ImageView>(ImageView::CI{
				.app = application(),
				.name = name() + ".GBuffer.position",
				.image_ci = Image::CI{
					.app = application(),
					.name = name() + ".GBuffer.position",
					.type = _target->image()->type(),
					.format = VK_FORMAT_R32G32B32A32_SFLOAT,
					.extent = _target->image()->extent(),
					.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
					.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				},
			});
			_exec.declare(_deferred_pipeline._position);

			_deferred_pipeline._normal = std::make_shared<ImageView>(ImageView::CI{
				.app = application(),
				.name = name() + ".GBuffer.normal",
				.image_ci = Image::CI{
					.app = application(),
					.name = name() + ".GBuffer.normal",
					.type = _target->image()->type(),
					.format = VK_FORMAT_R32G32B32A32_SFLOAT,
					.extent = _target->image()->extent(),
					.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
					.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				},
			});
			_exec.declare(_deferred_pipeline._normal);

			for (uint32_t model_type : _model_types)
			{
				_deferred_pipeline._raster_gbuffer[model_type] = std::make_shared<VertexCommand>(VertexCommand::CI{
					.app = application(),
					.name = name() + ".RasterGBuffer",
					.vertex_input_desc = RigidMesh::vertexInputDescFullVertex(),
					.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
					.sets_layouts = (_sets_layouts + std::pair{model_set, model_layout[model_type]}),
					.bindings = {
						Binding{
							.buffer = _ubo_buffer,
							.binding = 0,
						},
					},
					.color_attachements = {_deferred_pipeline._albedo, _deferred_pipeline._position, _deferred_pipeline._normal},
					.depth_buffer = _depth,
					.write_depth = true,
					.vertex_shader_path = shaders / "RasterGBuffer.vert",
					.fragment_shader_path = shaders / "RasterGBuffer.frag",
					.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
					.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
				});
				_exec.declare(_deferred_pipeline._raster_gbuffer[model_type]);
			}

			_deferred_pipeline._shade_from_gbuffer = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = application(),
				.name = name() + ".ShadeFromGBuffer",
				.shader_path = shaders / "ShadeFromGBuffer.comp",
				.dispatch_size = _target->image()->extent(),
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.view = _deferred_pipeline._albedo,
						.binding = 0,
					},
					Binding{
						.view = _deferred_pipeline._position,
						.binding = 1,
					},
					Binding{
						.view = _deferred_pipeline._normal,
						.binding = 2,
					},
					Binding{
						.view = _target,
						.binding = 4,
					},
				},
			});
			_exec.declare(_deferred_pipeline._shade_from_gbuffer);
		}


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
		_exec.declare(_render_3D_basis);

		_update_buffer = std::make_shared<UpdateBuffer>(UpdateBuffer::CI{
				.app = application(),
				.name = "UpdateUBO",
			});
		_exec.declare(_update_buffer);
	}

	SimpleRenderer::MultiVertexDrawCallList SimpleRenderer::generateVertexDrawList()
	{
		MultiVertexDrawCallList res;

		auto add_model = [&res](std::shared_ptr<Scene::Node> const& node, glm::mat4 const& matrix)
		{
			if (node->visible() && node->model())
			{
				const uint32_t model_type = node->model()->type();
				VertexCommand::DrawCallInfo draw_call;
				Drawable::VertexDrawCallResources * vr = reinterpret_cast<Drawable::VertexDrawCallResources *>(&draw_call.draw_count);
				assert(vr);
				node->model()->fillVertexDrawCallResources(*vr);

				draw_call.set = node->model()->setAndPool();
				draw_call.pc = matrix,

				res[model_type].push_back(draw_call);
			}
			return node->visible();
		};

		_scene->getTree()->iterateOnDag(add_model);
		return res;
	}

	void SimpleRenderer::execute(Camera const& camera, float time, float dt, uint32_t frame_id)
	{
		UBO ubo{
			.time = time,
			.delta_time = dt,
			.frame_idx = frame_id,
			
			.world_to_camera = camera.getWorldToCam(),
			.camera_to_proj = camera.getCamToProj(),
			.world_to_proj = camera.getWorldToProj(),
		};

		_exec(_update_buffer->with(UpdateBuffer::UpdateInfo{
			.src = ubo,
			.dst = _ubo_buffer,
		}));

		MultiVertexDrawCallList draw_list = generateVertexDrawList();

		if (!draw_list.empty())
		{
			const size_t selected_pipeline = _pipeline_selection.index();
			if (selected_pipeline == 0)
			{
				for (uint32_t model_type : _model_types)
				{
					if (!draw_list[model_type].empty())
					{
						_exec(_direct_pipeline._render_scene_direct[model_type]->with(VertexCommand::DrawInfo{
							.draw_type = VertexCommand::DrawType::DrawIndexed,
							.draw_list = draw_list[model_type],
						}));
					}
				}
			}
			else
			{
				for (uint32_t model_type : _model_types)
				{
					if (!draw_list[model_type].empty())
					{
						_exec(_deferred_pipeline._raster_gbuffer[model_type]->with(VertexCommand::DrawInfo{
							.draw_type = VertexCommand::DrawType::DrawIndexed,
							.draw_list = draw_list[model_type],
						}));
					}
				}
				_exec(_deferred_pipeline._shade_from_gbuffer);
			}
		}

		{
			std::vector<VertexCommand::DrawCallInfo> basis_draw_list;
			using namespace std::containers_operators;
			if (_show_world_3D_basis)
			{
				basis_draw_list += VertexCommand::DrawCallInfo{
					.draw_count = 3,
					.pc = camera.getWorldToProj(),
				};
			}
			if (_show_view_3D_basis)
			{
				glm::mat4 view_3D_basis_matrix = camera.getCamToProj() * translateMatrix<4, float>(glm::vec3(0, 0, -0.25)) * camera.getWorldRoationMatrix() * scaleMatrix<4, float>(0.03125);
				basis_draw_list += VertexCommand::DrawCallInfo{
					.draw_count = 3,
					.pc = view_3D_basis_matrix,
				};
			}
			if (!basis_draw_list.empty())
			{
				_exec(_render_3D_basis->with(VertexCommand::DrawInfo{
					.draw_type = VertexCommand::DrawType::Draw,
					.draw_list = basis_draw_list,
				}));
			}
		}
	}
	

	void SimpleRenderer::declareImGui()
	{
		if (ImGui::CollapsingHeader(name().c_str()))
		{
			_pipeline_selection.declare();
			ImGui::Checkbox("show world 3D basis", &_show_world_3D_basis);
			ImGui::Checkbox("show view 3D basis", &_show_view_3D_basis);
		}
	}
}