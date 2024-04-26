#include "Renderer.hpp"

#include <Core/Commands/PrebuiltTransferCommands.hpp>

namespace vkl
{
	SimpleRenderer::SimpleRenderer(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_sets_layouts(ci.sets_layouts),
		_scene(ci.scene),
		_output_target(ci.target) 
	{
		_pipeline_selection = ImGuiListSelection::CI{
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.labels = {"Forward V1"s, "Deferred V1"s, "Path Tacing"s},
			.default_index = 1,
			.same_line = true,
		};

		_shadow_method = ImGuiListSelection::CI{
			.name = "Shadows",
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.labels = {"None", "Shadow Maps", "Ray Tracing"},
			.default_index = 1,
			.same_line = true,
		};
		_shadow_method_glsl_def = "SHADING_SHADOW_METHOD 0";
		_use_ao_glsl_def = "USE_AO 0";

		const bool can_multi_draw_indirect = application()->availableFeatures().features2.features.multiDrawIndirect;
		_use_indirect_rendering = can_multi_draw_indirect;

		const bool can_as = application()->availableFeatures().acceleration_structure_khr.accelerationStructure;
		const bool can_rq = application()->availableFeatures().ray_query_khr.rayQuery;
		const bool can_rt = application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline;
		
		_pipeline_selection.enableOptions(2, can_as && (can_rq || can_rt));
		if (!can_as && (can_rq || can_rt) && RenderPipeline(_pipeline_selection.index()) == RenderPipeline::PathTaced)
		{
			_pipeline_selection.setIndex(1);
		}
		_shadow_method.enableOptions(size_t(ShadowMethod::RayTraced), can_as && (can_rq || can_rt));
		if (!can_as && (can_rq || can_rt) && _shadow_method.index() == size_t(ShadowMethod::RayTraced))
		{
			_shadow_method.setIndex(size_t(ShadowMethod::ShadowMap));
		}

		_maintain_rt = false;

		createInternalResources();
	}

	void SimpleRenderer::updateMaintainRT()
	{
		const bool can_as = application()->availableFeatures().acceleration_structure_khr.accelerationStructure;
		const bool can_rq = application()->availableFeatures().ray_query_khr.rayQuery;
		const bool can_rt = application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline;
		
		bool need_rq = false;
		bool need_rt = false;
		if (RenderPipeline(_pipeline_selection.index()) == RenderPipeline::PathTaced)
		{
			need_rq |= true;
		}
		if (_ambient_occlusion)
		{
			uint32_t ao_needs = _ambient_occlusion->needRTOrRQ();
			need_rt |= (ao_needs & 0x1) != 0;
			need_rq |= (ao_needs & 0x2) != 0;
		}
		if (_shadow_method.index() == size_t(ShadowMethod::RayTraced))
		{
			need_rq |= true;
		}

		
		if (can_as && (need_rq || need_rt))
		{
			_maintain_rt = true;
		}
		else
		{
			_maintain_rt = false;
		}
	}

	void SimpleRenderer::createInternalResources()
	{
		const bool can_as = application()->availableFeatures().acceleration_structure_khr.accelerationStructure;
		const bool can_rq = application()->availableFeatures().ray_query_khr.rayQuery;
		const bool can_rt = application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline;

		_render_target = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".render_target",
			.type = _output_target->image()->type(),
			.format = VK_FORMAT_R16G16B16A16_SFLOAT,
			.extent = _output_target->image()->extent(),
			.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_taau = std::make_shared<TemporalAntiAliasingAndUpscaler>(TemporalAntiAliasingAndUpscaler::CI{
			.app = application(),
			.name = name() + "TAAU",
			.input = _render_target,
			.sets_layouts = _sets_layouts,
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

		_light_depth_sampler = std::make_shared<Sampler>(Sampler::CI{
			.app = application(),
			.name = name() + "LightDepthSampler",
			.filter = VK_FILTER_LINEAR,
			.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.compare_op = VK_COMPARE_OP_LESS_OR_EQUAL,	
		});

		const size_t ubo_align = application()->deviceProperties().props2.properties.limits.minUniformBufferOffsetAlignment;

		_ubo_buffer = std::make_shared<HostManagedBuffer>(HostManagedBuffer::CI{
			.app = application(),
			.name = name() + ".ubo",
			.size = std::alignUp(sizeof(UBO), ubo_align) + std::alignUp(sizeof(PathTracer::UBO), ubo_align),
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
		_ubo = BufferSegment{
			.buffer = _ubo_buffer->buffer(),
			.range = Buffer::Range{.begin = 0, .len = std::alignUp(sizeof(UBO), ubo_align)},
		};
		
		_draw_indexed_indirect_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".draw_indirect_buffer",
			.size = [this](){
				const size_t align = application()->deviceProperties().props2.properties.limits.minStorageBufferOffsetAlignment;
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

		const std::filesystem::path shaders = application()->mountingPoints()["ProjectShaders"];

		_prepare_draw_list = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".PrepareDrawList",
			.shader_path = shaders / "PrepareIndirectDrawList.comp",
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.buffer = _vk_draw_params_segment,
					.binding = 0,
				},
				Binding{
					.buffer = _model_indices_segment,
					.binding = 1,
				},
				Binding{
					.buffer = _atomic_counter_segment,
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
				.cull_mode = VK_CULL_MODE_BACK_BIT,
				.sets_layouts = (_sets_layouts + std::pair{model_set, model_layout[model_type]}),
				.bindings = {
					Binding{
						.buffer = _ubo,
						.binding = 0,
					},
					Binding{
						.sampler = _light_depth_sampler,
						.binding = 6,
					},
				},
				.color_attachements = {_render_target},
				.depth_stencil = _depth,
				.write_depth = true,
				.depth_compare_op = VK_COMPARE_OP_LESS,
				.vertex_shader_path = shaders / "render.vert",
				.fragment_shader_path = shaders / "render.frag",
				.definitions = [this](){DefinitionsList res; res.push_back(_shadow_method_glsl_def); return res;},
				.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
				.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
			});
		}

		_direct_pipeline._render_scene_indirect = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".RenderSceneIndirect",
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.cull_mode = VK_CULL_MODE_BACK_BIT,
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.buffer = _ubo,
					.binding = 0,
				},
				Binding{
					.buffer = _model_indices_segment,
					.binding = 1,
				},
				Binding{
					.sampler = _light_depth_sampler,
					.binding = 6,
				},
			},
			.color_attachements = {_render_target},
			.depth_stencil = _depth,
			.write_depth = true,
			.depth_compare_op = VK_COMPARE_OP_LESS,
			.vertex_shader_path = shaders / "RenderIndirect.vert",
			.fragment_shader_path = shaders / "RenderIndirect.frag",
			.definitions = [this]() {DefinitionsList res; res.push_back(_shadow_method_glsl_def); return res; },
			.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
			.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
		});

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

			_deferred_pipeline._tangent = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.tangent",
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
					.cull_mode = VK_CULL_MODE_BACK_BIT,
					.sets_layouts = (_sets_layouts + std::pair{model_set, model_layout[model_type]}),
					.bindings = {
						Binding{
							.buffer = _ubo,
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
				.cull_mode = VK_CULL_MODE_BACK_BIT,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _ubo,
						.binding = 0,
					},
					Binding{
						.buffer = _model_indices_segment,
						.binding = 1,
					},
				},
				.color_attachements = {_deferred_pipeline._albedo, _deferred_pipeline._position, _deferred_pipeline._normal, _deferred_pipeline._tangent},
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
				.can_rt = can_as && (can_rt || can_rq),
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
						.image = _deferred_pipeline._albedo,
						.binding = 0,
					},
					Binding{
						.image = _deferred_pipeline._position,
						.binding = 1,
					},
					Binding{
						.image = _deferred_pipeline._normal,
						.binding = 2,
					},
					Binding{
						.image = _deferred_pipeline._tangent,
						.binding = 3,
					},
					Binding{
						.image = _ambient_occlusion->target(),
						.sampler = bilinear_sampler,
						.binding = 4,
					},
					Binding{
						.image = _render_target,
						.binding = 5,
					},
					Binding{
						.sampler = _light_depth_sampler,
						.binding = 6,
					},
				},
				.definitions = [this]()
				{
					DefinitionsList res = {
						_use_ao_glsl_def,
						_shadow_method_glsl_def,
					};
					return res;
				},
			});

			_render_spot_light_depth = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RenderSpotLightDepth",
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.cull_mode = VK_CULL_MODE_FRONT_BIT, // front because the light matrix make the image flipped, which inverts the culling
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _model_indices_segment,
						.binding = 1,
					},
				},
				.extern_framebuffer = ExternFramebufferInfo{
					.detph_stencil_attchement = AttachmentInfo{
						.format = [this](){return _scene->lightDepthFormat();},
						.samples = [this](){return _scene->lightDepthSamples();},
					},
				},
				.write_depth = true,
				.depth_compare_op = VK_COMPARE_OP_LESS,
				.vertex_shader_path = shaders / "RasterSceneDepth.vert",
				.fragment_shader_path = shaders / "RasterSceneDepth.frag",
				.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1},
			});

			_render_point_light_depth = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RenderPointLightDepth",
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.cull_mode = VK_CULL_MODE_FRONT_BIT, // front because the light matrix make the image flipped, which inverts the culling
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _model_indices_segment,
						.binding = 1,
					},
				},
				.extern_framebuffer = ExternFramebufferInfo{
					.detph_stencil_attchement = AttachmentInfo{
						.format = [this]() {return _scene->lightDepthFormat(); },
						.samples = [this]() {return _scene->lightDepthSamples(); },
					},
					.layers = 1,
					.multiview = true,
				},
				.write_depth = true,
				.depth_compare_op = VK_COMPARE_OP_LESS,
				.vertex_shader_path = shaders / "RasterSceneDepth.vert",
				.fragment_shader_path = shaders / "RasterSceneDepth.frag",
				.definitions = MyVector{"TARGET_CUBE 1"s},
				.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1},
			});
		}

		if (application()->availableFeatures().acceleration_structure_khr.accelerationStructure)
		{
			_build_as = std::make_shared<BuildAccelerationStructureCommand>(BuildAccelerationStructureCommand::CI{
				.app = application(),
				.name = name() + ".BuildAS",
			});

			_path_tracer._ubo = BufferSegment{
				.buffer = _ubo_buffer->buffer(),
				.range = Buffer::Range{.begin = std::alignUp(sizeof(UBO), ubo_align), .len = std::alignUp(sizeof(PathTracer::UBO), ubo_align)},
			};

			_path_tracer._path_trace = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = application(),
				.name = name() + ".PathTracer",
				.shader_path = shaders / "RT/PathTracing.comp",
				.extent = _render_target->image()->extent(),
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _ubo,
						.binding = 0,
					},
					Binding{
						.buffer = _path_tracer._ubo,
						.binding = 1,
					},
					Binding{
						.image = _render_target,
						.binding = 2,
					},
				},
			});
		}
	}

	void SimpleRenderer::FillASBuildLists()
	{
		_blas_build_list.clear();
		
		auto process_mesh = [&](std::shared_ptr<Scene::Node> const& node, glm::mat4 const& matrix)
		{
			if (node->visible() && node->model() && node->model()->isReadyToDraw())
			{
				std::shared_ptr<Mesh> const& mesh = node->model()->mesh();
				if (mesh->isReadyToDraw() && mesh->blas())
				{
					_blas_build_list.pushIFN(mesh->blas());
				}
			}
			return node->visible();
		};
		_scene->getTree()->iterateOnDag(process_mesh);
	}

	void SimpleRenderer::generateVertexDrawList(MultiVertexDrawCallList & res)
	{
		VertexDrawCallInfo vr;
		const bool can_as = application()->availableFeatures().acceleration_structure_khr.accelerationStructure;
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

	struct LightInstanceData : public Scene::LightInstanceSpecificData
	{
		std::shared_ptr<Framebuffer> framebuffer;
	};

	void SimpleRenderer::preUpdate(UpdateContext& ctx)
	{
		updateMaintainRT();
		_scene->setMaintainRT(_maintain_rt);
	}

	void SimpleRenderer::updateResources(UpdateContext & ctx)
	{
		bool update_all_anyway = ctx.updateAnyway();

		_use_ao_glsl_def.back() = '0' + (_ambient_occlusion->enable() ? 1 : 0);
		_shadow_method_glsl_def.back() = '0' + _shadow_method.index();

		_taau->updateResources(ctx);
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

		if (RenderPipeline(_pipeline_selection.index()) == RenderPipeline::Forward || update_all_anyway)
		{
			if (_use_indirect_rendering || update_all_anyway)
			{
				ctx.resourcesToUpdateLater() += _direct_pipeline._render_scene_indirect;
			}
			if (!_use_indirect_rendering || update_all_anyway)
			{
				for (auto& cmd : _direct_pipeline._render_scene_direct)
				{
					ctx.resourcesToUpdateLater() += cmd.second;
				}
			}
		}
		if (RenderPipeline(_pipeline_selection.index()) == RenderPipeline::Deferred || update_all_anyway)
		{
			_deferred_pipeline._albedo->updateResource(ctx);
			_deferred_pipeline._position->updateResource(ctx);
			_deferred_pipeline._normal->updateResource(ctx);
			_deferred_pipeline._tangent->updateResource(ctx);

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
		if ((RenderPipeline(_pipeline_selection.index()) == RenderPipeline::PathTaced || update_all_anyway) && application()->availableFeatures().acceleration_structure_khr.accelerationStructure)
		{
			_path_tracer._path_trace->updateResources(ctx);
		}
		
		if (_use_indirect_rendering)
		{
			ctx.resourcesToUpdateLater() += _render_spot_light_depth;
			ctx.resourcesToUpdateLater() += _render_point_light_depth;
		}
		else
		{

		}

		{
			for (auto& [path, lid] : _scene->_unique_light_instances)
			{
				std::shared_ptr<Light> const& light = path.path.back()->light();
				{
					if (!lid.specific_data)
					{
						lid.specific_data = std::make_unique<LightInstanceData>();
					}

					LightInstanceData * my_lid = dynamic_cast<LightInstanceData*>(lid.specific_data.get());
					if(my_lid)
					{
						if (light->enableShadowMap())
						{
							if (!my_lid->framebuffer)
							{
								std::shared_ptr<RenderPass> render_pass;
								if (light->type() == LightType::POINT)
								{
									render_pass = _render_point_light_depth->renderPass();
								}
								else if (light->type() == LightType::SPOT)
								{
									render_pass = _render_spot_light_depth->renderPass();
								}
								my_lid->framebuffer = std::make_shared<Framebuffer>(Framebuffer::CI{
									.app = application(),
									.name = lid.depth_view->name(),
									.render_pass = render_pass,
									.depth_stencil = lid.depth_view,
									.layers = 1,
								});
							}
						}
						else
						{
							my_lid->framebuffer.reset();
						}
					}

					if (lid.depth_view)
					{
						lid.depth_view->updateResource(ctx);
					}
					if (my_lid && my_lid->framebuffer && _use_indirect_rendering)
					{
						ctx.resourcesToUpdateLater() += my_lid->framebuffer;
					}
				}
			}
		}

		_light_depth_sampler->updateResources(ctx);

		_ubo_buffer->updateResources(ctx);
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
		_ubo_buffer->set(0, &ubo, sizeof(ubo));

		_ubo_buffer->recordTransferIFN(exec);

		std::TickTock_hrc tick_tock;
		MultiVertexDrawCallList & draw_list = _cached_draw_list;

		const bool needs_draw_list = _pipeline_selection.index() <= size_t(RenderPipeline::Deferred);
		const bool generate_indirect_draw_list = needs_draw_list && _use_indirect_rendering;
		const bool generate_host_draw_list = needs_draw_list && !_use_indirect_rendering;
		
		if (generate_indirect_draw_list)
		{
			if (_cached_draw_list.empty())
			{
				_cached_draw_list.operator[](0);
			}
			FillBuffer & fill_buffer = application()->getPrebuiltTransferCommands().fill_buffer;
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
		else if(generate_host_draw_list)
		{
			tick_tock.tick();
			generateVertexDrawList(draw_list);
			if (exec.framePerfCounters())
			{
				exec.framePerfCounters()->generate_scene_draw_list_time = tick_tock.tockv().count();
			}
		}

		if (_maintain_rt)
		{
			assert(application()->availableFeatures().acceleration_structure_khr.accelerationStructure);
			FillASBuildLists();
			if (!_blas_build_list.empty())
			{
				exec(_build_as->with(_blas_build_list));
			}
			_scene->buildTLAS(exec);
		}

		const bool render_shadows = _pipeline_selection.index() < 2 && _shadow_method.index() == size_t(ShadowMethod::ShadowMap);
		if(render_shadows)
		{
			if (_use_indirect_rendering)
			{
				bool pushed_label = false;
				VertexCommand::DrawInfo& my_draw_list = (_cached_draw_list.begin())->second;
				std::shared_ptr<Framebuffer> previous_fb = std::move(my_draw_list.extern_framebuffer);
				PushConstant previous_pc = std::move(my_draw_list.draw_list.drawCalls().front().pc);
				for (auto& [path, lid] : _scene->_unique_light_instances)
				{
					std::shared_ptr<Light> const& light = path.path.back()->light();
					LightInstanceData * my_lid = dynamic_cast<LightInstanceData*>(lid.specific_data.get());
					if (light->enableShadowMap() && my_lid && my_lid->framebuffer && ((lid.flags & 1) != 0))
					{
						my_draw_list.draw_list.drawCalls().front().pc = lid.frame_light_id;
						my_draw_list.extern_framebuffer = my_lid->framebuffer;

						if (!pushed_label)
						{
							exec.pushDebugLabel("RenderShadowMaps");
							pushed_label = true;
						}
						if (light->type() == LightType::SPOT)
						{
							exec(_render_spot_light_depth->with(my_draw_list));
						}
						else if (light->type() == LightType::POINT)
						{
							exec(_render_point_light_depth->with(my_draw_list));
						}
					}
				}
				my_draw_list.extern_framebuffer = std::move(previous_fb);
				my_draw_list.draw_list.drawCalls().front().pc = std::move(previous_pc);

				if (pushed_label)
				{
					exec.popDebugLabel();
				}
			}
		}

		if (!draw_list.empty() && _pipeline_selection.index() < 2)
		{
			const size_t selected_pipeline = _pipeline_selection.index();
			if (selected_pipeline == 0)
			{
				tick_tock.tick();
				exec.pushDebugLabel("DirectPipeline");
				if (_use_indirect_rendering)
				{
					VertexCommand::DrawInfo& my_draw_list = (_cached_draw_list.begin())->second;
					exec(_direct_pipeline._render_scene_indirect->with(my_draw_list));
				}
				else
				{
					for (uint32_t model_type : _model_types)
					{
						if (draw_list[model_type].draw_list.size())
						{
							exec(_direct_pipeline._render_scene_direct[model_type]->with(draw_list[model_type]));
						}
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

		if (RenderPipeline(_pipeline_selection.index()) == RenderPipeline::PathTaced)
		{
			exec(_path_tracer._path_trace);
		}

		_taau->execute(exec, camera);

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
		ImGui::PushID(this);
		if (ImGui::CollapsingHeader(name().c_str()))
		{
			const bool can_multi_draw_indirect = application()->availableFeatures().features2.features.multiDrawIndirect;
			ImGui::BeginDisabled(!can_multi_draw_indirect);
			ImGui::Checkbox("Indirect Draw", &_use_indirect_rendering);
			ImGui::EndDisabled();

			_pipeline_selection.declare();

			if (RenderPipeline(_pipeline_selection.index()) == RenderPipeline::Deferred)
			{
				if (ImGui::CollapsingHeader(_ambient_occlusion->name().c_str()))
				{
					_ambient_occlusion->declareGui(ctx);
					ImGui::Separator();
				}
			}
			
			ImGui::BeginDisabled(true);
			ImVec4 color;
			color = _maintain_rt ? ImVec4(0, 0.8, 0, 1) : ImVec4(0.8, 0, 0, 1);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::Checkbox("Ray Tracing", &_maintain_rt);
			ImGui::PopStyleColor(2);
			ImGui::EndDisabled();
			
			ImGui::PushID("shadow");
			_shadow_method.declare();
			ImGui::PopID();

			ImGui::Separator();

			if (ImGui::CollapsingHeader("TAAU"))
			{
				_taau->declareGui(ctx);
				ImGui::Separator();
			}
		}
		ImGui::PopID();
	}
}