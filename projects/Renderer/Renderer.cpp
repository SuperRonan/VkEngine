#include "Renderer.hpp"

namespace vkl
{
	SimpleRenderer::SimpleRenderer(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_sets_layouts(ci.sets_layouts),
		_scene(ci.scene),
		_output_target(ci.target) 
	{

		const bool can_multi_draw = application()->availableFeatures().features.multiDrawIndirect;
		if (!can_multi_draw)
		{
			_use_indirect_rendering = false;
		}

		createInternalResources();
		
	}

	void SimpleRenderer::createInternalResources()
	{
		_render_target = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".render_target",
			.type = _output_target->image()->type(),
			.format = VK_FORMAT_R16G16B16A16_SFLOAT,
			.extent = _output_target->image()->extent(),
			.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_depth = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".depth",
			.type = _render_target->image()->type(),
			.format = VK_FORMAT_D32_SFLOAT,
			.extent = _render_target->image()->extent(),
			.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});



		_ubo_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".ubo",
			.size = sizeof(UBO),
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
		
		_draw_indexed_indirect_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".draw_indirect_buffer",
			.size = [this](){
				const size_t align = application()->deviceProperties().props.limits.minStorageBufferOffsetAlignment;
				return std::alignUp(_model_capacity * (sizeof(VkDrawIndirectCommand) + sizeof(uint32_t)), align) + 4 * sizeof(uint32_t);
			},
			.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
		
		_vk_draw_params_segment = BufferAndRange{
			.buffer = _draw_indexed_indirect_buffer,
			.range = [this](){return Buffer::Range{.begin = 0, .len = _model_capacity * sizeof(VkDrawIndirectCommand)};},
		};

		_model_indices_segment = BufferAndRange{
			.buffer = _draw_indexed_indirect_buffer,
			.range = [this](){return Buffer::Range{.begin = _vk_draw_params_segment.range.value().len, .len = _model_capacity * sizeof(uint32_t)};}
		};

		_atomic_counter_segment = BufferAndRange{
			.buffer = _draw_indexed_indirect_buffer,
			.range = [this](){return Buffer::Range{.begin = _draw_indexed_indirect_buffer->size().value() - 4 * sizeof(uint32_t), .len = 4 * sizeof(uint32_t)};},
		};


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

		_prepare_draw_list = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".PrepareDrawList",
			.shader_path = shaders / "PrepareIndirectDrawList.comp",
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.buffer = _vk_draw_params_segment.buffer,
					.buffer_range = _vk_draw_params_segment.range,
					.binding = 0,
				},
				Binding{
					.buffer = _model_indices_segment.buffer,
					.buffer_range = _model_indices_segment.range,
					.binding = 1,
				},
				Binding{
					.buffer = _atomic_counter_segment.buffer,
					.buffer_range = _atomic_counter_segment.range,
					.binding = 2,
				},
			},
		});

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
				.color_attachements = {_render_target},
				.depth_stencil = _depth,
				.write_depth = true,
				.depth_compare_op = VK_COMPARE_OP_LESS,
				.vertex_shader_path = shaders / "render.vert",
				.fragment_shader_path = shaders / "render.frag",
				.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
				.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
			});
		}
		{
			_deferred_pipeline._albedo = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.albedo",
				.type = _render_target->image()->type(),
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.extent = _render_target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
			

			_deferred_pipeline._position = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.position",
				.type = _render_target->image()->type(),
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.extent = _render_target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
			

			_deferred_pipeline._normal = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.normal",
				.type = _render_target->image()->type(),
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.extent = _render_target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
			

			for (uint32_t model_type : _model_types)
			{
				DeferredPipelineV1::RasterCommands& raster_commands = _deferred_pipeline._raster_gbuffer[model_type];
				 raster_commands.raster = std::make_shared<VertexCommand>(VertexCommand::CI{
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
					.depth_stencil = _depth,
					.write_depth = true,
					.depth_compare_op = VK_COMPARE_OP_LESS,
					.vertex_shader_path = shaders / "RasterGBuffer.vert",
					.fragment_shader_path = shaders / "RasterGBuffer.frag",
					.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
					.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
				});
			}

			_deferred_pipeline.raster_gbuffer_indirect = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RasterSceneGBuffer",
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _ubo_buffer,
						.binding = 0,
					},
					Binding{
						.buffer = _model_indices_segment.buffer,
						.buffer_range = _model_indices_segment.range,
						.binding = 1,
					},
				},
				.color_attachements = {_deferred_pipeline._albedo, _deferred_pipeline._position, _deferred_pipeline._normal},
				.depth_stencil = _depth,
				.write_depth = true,
				.depth_compare_op = VK_COMPARE_OP_LESS,
				.vertex_shader_path = shaders / "RasterSceneGBuffer.vert",
				.fragment_shader_path = shaders / "RasterSceneGBuffer.frag",
				.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
				.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
			});
			
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
				.extent = _render_target->image()->extent(),
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
						.view = _render_target,
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

	void SimpleRenderer::generateVertexDrawList(MultiVertexDrawCallList & res)
	{
		

		VertexDrawCallInfo vr;
		auto add_model = [&res, &vr](std::shared_ptr<Scene::Node> const& node, glm::mat4 const& matrix)
		{
			if (node->visible() && node->model() && node->model()->isReadyToDraw())
			{
				vr.clear();
				const uint32_t model_type = node->model()->type();
				node->model()->fillVertexDrawCallInfo(vr);
				
				auto & res_model_type = res[model_type];
				res_model_type.draw_type = DrawType::DrawIndexed;
				res_model_type.draw_list.push_back(VertexDrawList::DrawCallInfo{
					.name = node->name(),
					.draw_count = vr.draw_count,
					.instance_count = vr.instance_count,
					.index_buffer = vr.index_buffer,
					.index_type = vr.index_type,
					.num_vertex_buffers = vr.vertex_buffers.size32(),
					.set = node->model()->setAndPool(),
					.pc = matrix,
				}, vr.vertex_buffers);
			}
			return node->visible();
		};
		_scene->getTree()->iterateOnDag(add_model);
	}

	void SimpleRenderer::updateResources(UpdateContext & ctx)
	{
		bool update_all_anyway = ctx.updateAnyway();

		_render_target->updateResource(ctx);
		_depth->updateResource(ctx);

		{
			const uint32_t desired_size = _scene->objectCount();
			if (desired_size > _model_capacity)
			{
				const uint32_t new_size = std::max(desired_size, 2 * _model_capacity);
				_model_capacity = new_size;
			}
		}
		if (_use_indirect_rendering || update_all_anyway)
		{
			_draw_indexed_indirect_buffer->updateResource(ctx);
			ctx.resourcesToUpdateLater() += _prepare_draw_list;
		}

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

			if (_use_indirect_rendering || update_all_anyway)
			{
				ctx.resourcesToUpdateLater() += _deferred_pipeline.raster_gbuffer_indirect;
			}
			if(!_use_indirect_rendering || update_all_anyway)
			{
				for (auto& cmd : _deferred_pipeline._raster_gbuffer)
				{
					auto & raster_commands = cmd.second;
					ctx.resourcesToUpdateLater() += raster_commands.raster;	
				}
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
		MultiVertexDrawCallList & draw_list = _cached_draw_list;
		
		if (_use_indirect_rendering)
		{
			FillBuffer fill_buffer = FillBuffer::CI{
				.app = application(),
			};
			exec(fill_buffer.with(FillBuffer::FillInfo{
				.buffer = _atomic_counter_segment.buffer,
				.range = _atomic_counter_segment.range.value(),
				.value = 0,
			}));

			const uint32_t num_objects = _scene->objectCount();
			struct PrepareDrawListPC
			{
				uint32_t num_objects;
			};
			exec(_prepare_draw_list->with(ComputeCommand::SingleDispatchInfo{
				.extent = VkExtent3D{.width = num_objects, .height = 1, .depth = 1},
				.dispatch_threads = true,
				.pc = PrepareDrawListPC{.num_objects = num_objects},
			}));
			VertexCommand::DrawInfo& my_draw_list = (_cached_draw_list.begin())->second;
			my_draw_list.draw_type = DrawType::IndirectDraw;
			my_draw_list.draw_list.push_back(VertexDrawList::DrawCallInfo{
				.draw_count = num_objects,
				.indirect_draw_buffer = _vk_draw_params_segment,
				.indirect_draw_stride = sizeof(VkDrawIndirectCommand),
			});
		}
		else
		{
			tick_tock.tick();
			generateVertexDrawList(draw_list);
			if (exec.framePerfCounters())
			{
				exec.framePerfCounters()->generate_scene_draw_list_time = tick_tock.tockv().count();
			}
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
					if (draw_list[model_type].draw_list.size())
					{
						exec(_direct_pipeline._render_scene_direct[model_type]->with(draw_list[model_type]));
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
				exec.pushDebugLabel("DeferredPipeline");
				
				tick_tock.tick();
				
				if (_use_indirect_rendering)
				{
					VertexCommand::DrawInfo & my_draw_list = (_cached_draw_list.begin())->second;
					{
						exec(_deferred_pipeline.raster_gbuffer_indirect->with(my_draw_list));
					}
				}
				else
				{
					for (uint32_t model_type : _model_types)
					{
						if (draw_list[model_type].draw_list.size())
						{
							exec(_deferred_pipeline._raster_gbuffer[model_type].raster->with(draw_list[model_type]));
						}
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

		{
			// Clear the cached draw list
			for (auto& [vt, vl] : _cached_draw_list)
			{
				vl.clear();
			}
		}

		exec.popDebugLabel();
	}
	

	void SimpleRenderer::declareGui(GuiContext & ctx)
	{
		if (ImGui::CollapsingHeader(name().c_str()))
		{
			const bool can_multi_draw = application()->availableFeatures().features.multiDrawIndirect;
			ImGui::BeginDisabled(!can_multi_draw);
			ImGui::Checkbox("Indirect Draw", &_use_indirect_rendering);
			ImGui::EndDisabled();

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