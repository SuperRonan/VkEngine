#include "Renderer.hpp"

namespace vkl
{
	SimpleRenderer::SimpleRenderer(CreateInfo const& ci):
		Module(ci.app, ci.name),
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



		_ubo_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".ubo",
			.size = sizeof(UBO),
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
		


		const uint32_t model_set = application()->descriptorBindingGlobalOptions().set_bindings[size_t(DescriptorSetName::invocation)].set;
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
				
			}
			
			_ambient_occlusion = std::make_shared<AmbientOcclusion>(AmbientOcclusion::CI{
				.app = application(),
				.name = name() + ".AO",
				.sets_layouts = _sets_layouts,
				.positions = _deferred_pipeline._position,
				.normals = _deferred_pipeline._normal,
			});

			std::shared_ptr<Sampler> bilinear_sampler = application()->getSamplerLibrary().getSampler(SamplerLibrary::SamplerInfo{
				.filter = VK_FILTER_LINEAR,
				.address_mode = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
			});

			_deferred_pipeline._shade_from_gbuffer = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = application(),
				.name = name() + ".ShadeFromGBuffer",
				.shader_path = shaders / "ShadeFromGBuffer.comp",
				.extent = _target->image()->extent(),
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
						.view = _ambient_occlusion->target(),
						.sampler = bilinear_sampler,
						.binding = 3,
					},
					Binding{
						.view = _target,
						.binding = 4,
					},
				},
				.definitions = [this]()
				{
					std::vector<std::string> res;
					res.push_back("USE_AO 0");
					if (_ambient_occlusion->enable())
					{
						res.back().back() = '1';
					}
					return res;
				},
			});

			
		}
	}

	SimpleRenderer::MultiVertexDrawCallList SimpleRenderer::generateVertexDrawList()
	{
		MultiVertexDrawCallList res;

		auto add_model = [&res](std::shared_ptr<Scene::Node> const& node, glm::mat4 const& matrix)
		{
			if (node->visible() && node->model() && node->model()->isReadyToDraw())
			{
				const uint32_t model_type = node->model()->type();
				VertexCommand::DrawCallInfo draw_call;
				Drawable::VertexDrawCallInfo & vr = draw_call.vertex_draw_info;
				node->model()->fillVertexDrawCallInfo(vr);

				draw_call.set = node->model()->setAndPool();
				draw_call.pc = matrix,
				draw_call.name = node->name();

				res[model_type].push_back(draw_call);
			}
			return node->visible();
		};

		_scene->getTree()->iterateOnDag(add_model);
		return res;
	}

	void SimpleRenderer::updateResources(UpdateContext & ctx)
	{
		bool update_all_anyway = ctx.updateAnyway();
		_depth->updateResource(ctx);

		if (_pipeline_selection.index() == 0 || update_all_anyway)
		{
			for (auto& cmd : _direct_pipeline._render_scene_direct)
			{
				ctx.resourcesToUpdateLater() += cmd.second;
			}
		}
		if (_pipeline_selection.index() == 1 || update_all_anyway)
		{
			_deferred_pipeline._albedo->updateResource(ctx);
			_deferred_pipeline._position->updateResource(ctx);
			_deferred_pipeline._normal->updateResource(ctx);

			for (auto& cmd : _deferred_pipeline._raster_gbuffer)
			{
				ctx.resourcesToUpdateLater() += cmd.second;
			}

			_ambient_occlusion->updateResources(ctx);

			ctx.resourcesToUpdateLater() += _deferred_pipeline._shade_from_gbuffer;
		}

		_ubo_buffer->updateResource(ctx);
	}

	void SimpleRenderer::execute(ExecutionRecorder& exec, Camera const& camera, float time, float dt, uint32_t frame_id)
	{
		exec.pushDebugLabel(name() + ".execute()");
		UBO ubo{
			.time = time,
			.delta_time = dt,
			.frame_idx = frame_id,
			
			.world_to_camera = camera.getWorldToCam(),
			.camera_to_proj = camera.getCamToProj(),
			.world_to_proj = camera.getWorldToProj(),
		};

		UpdateBuffer updater = UpdateBuffer::CI{
			.app = application(),
			.name = name() + ".UpdateUBO",
		};

		exec(updater.with(UpdateBuffer::UpdateInfo{
			.src = ubo,
			.dst = _ubo_buffer,
		}));

		std::TickTock_hrc tick_tock;
		tick_tock.tick();
		MultiVertexDrawCallList draw_list = generateVertexDrawList();
		if (exec.framePerfCounters())
		{
			exec.framePerfCounters()->generate_scene_draw_list_time = tick_tock.tockv().count();
		}

		if (!draw_list.empty())
		{
			const size_t selected_pipeline = _pipeline_selection.index();
			if (selected_pipeline == 0)
			{
				tick_tock.tick();
				exec.pushDebugLabel("DirectPipeline");
				for (uint32_t model_type : _model_types)
				{
					if (!draw_list[model_type].empty())
					{
						exec(_direct_pipeline._render_scene_direct[model_type]->with(VertexCommand::DrawInfo{
							.draw_type = VertexCommand::DrawType::DrawIndexed,
							.draw_list = draw_list[model_type],
						}));
					}
				}
				if (exec.framePerfCounters())
				{
					exec.framePerfCounters()->render_draw_list_time = tick_tock.tockv().count();
				}
				exec.popDebugLabel();
			}
			else
			{
				tick_tock.tick();
				exec.pushDebugLabel("DeferredPipeline");
				for (uint32_t model_type : _model_types)
				{
					if (!draw_list[model_type].empty())
					{
						exec(_deferred_pipeline._raster_gbuffer[model_type]->with(VertexCommand::DrawInfo{
							.draw_type = VertexCommand::DrawType::DrawIndexed,
							.draw_list = draw_list[model_type],
						}));
					}
				}
				if (exec.framePerfCounters())
				{
					exec.framePerfCounters()->render_draw_list_time = tick_tock.tockv().count();
				}

				_ambient_occlusion->execute(exec, camera);

				exec(_deferred_pipeline._shade_from_gbuffer);
				
				exec.popDebugLabel();
			}
		}
		exec.popDebugLabel();
	}
	

	void SimpleRenderer::declareGui(GuiContext & ctx)
	{
		if (ImGui::CollapsingHeader(name().c_str()))
		{
			_pipeline_selection.declare();

			if (_pipeline_selection.index() == 1)
			{
				if (ImGui::CollapsingHeader(_ambient_occlusion->name().c_str()))
				{
					_ambient_occlusion->declareGui(ctx);
				}
			}
		}
	}
}