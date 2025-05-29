#include "Renderer.hpp"

#include <vkl/Commands/PrebuiltTransferCommands.hpp>

namespace vkl
{
	SimpleRenderer::SimpleRenderer(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_sets_layouts(ci.sets_layouts),
		_scene(ci.scene),
		_output_target(ci.target),
		_camera(ci.camera)
	{
		_pipeline_selection = ImGuiListSelection::CI{
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.same_line = true,
			.options = {
				ImGuiListSelection::Option{
					.name = "Forward V1",
				},
				ImGuiListSelection::Option{
					.name = "Deferred V1",
					.desc = "Deferred Rendering is currently on maintenance",
					.disable = true,
				},
				ImGuiListSelection::Option{
					.name = "Light Transport",
				},
			},
			.default_index = 0,
		};

		_shadow_method = ImGuiListSelection::CI{
			.name = "Shadows",
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.same_line = true,
			.labels = {"None", "Shadow Maps", "Ray Tracing"},
			.default_index = 1,
		};
		_shadow_method_glsl_def = "SHADING_SHADOW_METHOD 0";
		_use_ao_glsl_def = "USE_AO 0";

		const bool can_multi_draw_indirect = application()->availableFeatures().features2.features.multiDrawIndirect;
		_use_indirect_rendering = can_multi_draw_indirect;

		const bool can_as = application()->availableFeatures().acceleration_structure_khr.accelerationStructure;
		const bool can_rq = application()->availableFeatures().ray_query_khr.rayQuery;
		const bool can_rt = application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline;

		if (can_as && (can_rq || can_rt))
		{
			_pipeline_selection.setIndex(2);
		}
		
		_pipeline_selection.enableOptions(2, can_as && (can_rq || can_rt));
		if (!can_as && (can_rq || can_rt) && RenderPipeline(_pipeline_selection.index()) == RenderPipeline::LightTransport)
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
		if (RenderPipeline(_pipeline_selection.index()) == RenderPipeline::LightTransport)
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

		{
			MyVector<DescriptorSetLayout::Binding> layout_bindings;

			layout_bindings += DescriptorSetLayout::Binding{
				.binding = 0,
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.stages = VK_SHADER_STAGE_ALL,
				.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			};

			layout_bindings += DescriptorSetLayout::Binding{
				.binding = 1,
				.type = VK_DESCRIPTOR_TYPE_SAMPLER,
				.stages = VK_SHADER_STAGE_ALL,
			};

			_set_layout = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
				.app = application(),
				.name = name() + ".SetLayout",
				.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
				.is_dynamic = false,
				.bindings = std::move(layout_bindings),
				.binding_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
			});

			_sets_layouts.set(application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::module)].set, _set_layout);
		}

		_render_target = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".render_target",
			.type = _output_target->image()->type(),
			.format = _output_target->format(),
			.extent = _output_target->image()->extent(),
			.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_taau = std::make_shared<TemporalAntiAliasingAndUpscaler>(TemporalAntiAliasingAndUpscaler::CI{
			.app = application(),
			.name = name() + ".TAAU",
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
		size_t ubo_size = std::alignUp(sizeof(UBO), ubo_align);
		ubo_size += std::alignUp(sizeof(LightTransport::UBO), ubo_align);
		_ubo_buffer = std::make_shared<HostManagedBuffer>(HostManagedBuffer::CI{
			.app = application(),
			.name = name() + ".ubo",
			.size = ubo_size,
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
		_ubo = BufferSegment{
			.buffer = _ubo_buffer->buffer(),
			.range = Buffer::Range{.begin = 0, .len = std::alignUp(sizeof(UBO), ubo_align)},
		};

		_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CI{
			.app = application(),
			.name = name() + ".Set",
			.layout = _set_layout,
			.bindings = {
				Binding{
					.buffer = _ubo,
					.binding = 0,
				},
				Binding{
					.sampler = _light_depth_sampler,
					.binding = 1,
				},
			},
		});
		
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

		const std::filesystem::path shaders = "RenderLibShaders:/";

		_prepare_draw_list = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".PrepareDrawList",
			.shader_path = shaders / "PrepareIndirectDrawList.slang",
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

		_forward_pipeline.render_pass = std::make_shared<RenderPass>(RenderPass::CI{
			.app = application(),
			.name = name() + ".DirectPipeline.RenderPass",
			.attachments = {
				AttachmentDescription2{
					.flags = AttachmentDescription2::Flags::Clear,
					.format = _render_target->format(),
					.samples = _render_target->image()->sampleCount(),
				},
				AttachmentDescription2{
					.flags = AttachmentDescription2::Flags::Clear,
					.format = _depth->format(),
					.samples = _depth->image()->sampleCount(),
				},
			},
			.subpasses = {
				SubPassDescription2{
					.colors = {AttachmentReference2{.index = 0}},
					.depth_stencil = AttachmentReference2{.index = 1},
				}
			},
		});

		_forward_pipeline.framebuffer = std::make_shared<Framebuffer>(Framebuffer::CI{
			.app = application(),
			.name = name() + ".DirectPipeline.Framebuffer",
			.render_pass = _forward_pipeline.render_pass,
			.attachments = {_render_target, _depth},
		});

		for (uint32_t model_type : _model_types)
		{
			_forward_pipeline.render_scene_direct[model_type] = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RenderSceneDirect",
				.vertex_input_desc = RigidMesh::vertexInputDescFullVertex(), // TODO model type dependent
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.cull_mode = VK_CULL_MODE_BACK_BIT,
				.sets_layouts = (_sets_layouts + std::pair{model_set, model_layout[model_type]}),
				.bindings = {
				},
				.extern_render_pass = _forward_pipeline.render_pass,
				.write_depth = true,
				.depth_compare_op = VK_COMPARE_OP_LESS,
				.vertex_shader_path = shaders / "render.vert.slang",
				.fragment_shader_path = shaders / "render.frag.slang",
				.definitions = [this](DefinitionsList & res){res = {_shadow_method_glsl_def};},
			});
		}

		_forward_pipeline.render_scene_indirect = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".RenderSceneIndirect",
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.cull_mode = VK_CULL_MODE_BACK_BIT,
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.buffer = _model_indices_segment,
					.binding = 0,
				},
			},
			.extern_render_pass = _forward_pipeline.render_pass,
			.write_depth = true,
			.depth_compare_op = VK_COMPARE_OP_LESS,
			.vertex_shader_path = shaders / "RenderIndirect.vert.slang",
			.fragment_shader_path = shaders / "RenderIndirect.frag.slang",
			.definitions = [this](DefinitionsList & res) {res = {_shadow_method_glsl_def};},
		});

		{
			Dyn<bool> hold_fat_deferred = true;
			Dyn<bool> hold_minimal_deferred = false;
			_fat_deferred_pipeline.albedo = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.albedo",
				.type = _render_target->image()->type(),
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.extent = _render_target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.hold_instance = hold_fat_deferred,
			});
			
			_fat_deferred_pipeline.position = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.position",
				.type = _render_target->image()->type(),
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.extent = _render_target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.hold_instance = hold_fat_deferred,
			});
			
			_fat_deferred_pipeline.normal = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.normal",
				.type = _render_target->image()->type(),
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.extent = _render_target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.hold_instance = hold_fat_deferred,
			});

			_fat_deferred_pipeline.tangent = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.tangent",
				.type = _render_target->image()->type(),
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.extent = _render_target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.hold_instance = hold_fat_deferred,
			});

			_fat_deferred_pipeline.render_pass = std::make_shared<RenderPass>(RenderPass::CI{
				.app = application(),
				.name = name() + ".Deferred.FatRenderPass",
				.attachments = {
					AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Clear, _fat_deferred_pipeline.albedo),
					AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Clear, _fat_deferred_pipeline.position),
					AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Clear, _fat_deferred_pipeline.normal),
					AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Clear, _fat_deferred_pipeline.tangent),
					AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Clear, _depth),
				},
				.subpasses = {
					SubPassDescription2{
						.colors = {AttachmentReference2{.index = 0}, AttachmentReference2{.index = 1}, AttachmentReference2{.index = 2}, AttachmentReference2{.index = 3}},
						.depth_stencil = AttachmentReference2{.index = 4},
					}
				},
				.hold_instance = hold_fat_deferred,
			});

			_fat_deferred_pipeline.framebuffer = std::make_shared<Framebuffer>(Framebuffer::CI{
				.app = application(),
				.name = name() + ".Deferred.FatFramebuffer",
				.render_pass = _fat_deferred_pipeline.render_pass,
				.attachments = {_fat_deferred_pipeline.albedo, _fat_deferred_pipeline.position, _fat_deferred_pipeline.normal, _fat_deferred_pipeline.tangent, _depth},
				.hold_instance = hold_fat_deferred,
			});
			

			_minimal_deferred_pipeline.ids = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.Ids",
				.type = _render_target->image()->type(),
				.format = VK_FORMAT_R32G32_UINT,
				.extent = _render_target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.hold_instance = hold_minimal_deferred,
			});

			_minimal_deferred_pipeline.uvs = std::make_shared<ImageView>(Image::CI{
				.app = application(),
				.name = name() + ".GBuffer.uvs",
				.type = _render_target->image()->type(),
				.format = VK_FORMAT_R16G16_UNORM,
				.extent = _render_target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.hold_instance = hold_minimal_deferred,
			});

			_minimal_deferred_pipeline.render_pass = std::make_shared<RenderPass>(RenderPass::CI{
				.app = application(),
				.name = name() + ".Deferred.MinimalRenderPass",
				.attachments = {
					AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Clear, _minimal_deferred_pipeline.ids),
					AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Clear, _minimal_deferred_pipeline.uvs),
					AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Clear, _depth),
				},
				.subpasses = {
					SubPassDescription2{
						.colors = {AttachmentReference2{.index = 0}, AttachmentReference2{.index = 1}},
						.depth_stencil = AttachmentReference2{.index = 2},
					}
				},
				.hold_instance = hold_minimal_deferred,
			});

			_minimal_deferred_pipeline.framebuffer = std::make_shared<Framebuffer>(Framebuffer::CI{
				.app = application(),
				.name = name() + ".Deferred.MinimalFramebuffer",
				.render_pass = _minimal_deferred_pipeline.render_pass,
				.attachments = {_minimal_deferred_pipeline.ids, _minimal_deferred_pipeline.uvs, _depth},
				.hold_instance = hold_minimal_deferred,
			});

			const Dyn<DefinitionsList> fat_definitions = [this](DefinitionsList& res)
			{
				res.clear();
				res.pushBack("GBUFFER_MODE GBUFFER_MODE_FAT");
			};

			const Dyn<DefinitionsList> minimal_definitions = [this](DefinitionsList& res)
			{
				res.clear();
				res.pushBack("GBUFFER_MODE GBUFFER_MODE_MINIMAL");
			};

			const auto derive_command_ci = [&] (
				auto & target,
				std::string_view name,
				MyVector<Binding> const& bindings = {},
				Dyn<DefinitionsList> const& definitions = {}
			)
			{
				target.name = name;
				target.bindings += bindings;
				if (definitions)
				{
					if (target.definitions)
					{
						target.definitions += definitions;
					}
					else
					{
						target.definitions = definitions;
					}
				}
			};

			const auto create_derived_vertex_command = [&] (
				VertexCommand::CI const& base,
				std::string_view name,
				std::optional<std::shared_ptr<RenderPass>> const& render_pass = {},
				MyVector<Binding> const& bindings = {},
				Dyn<DefinitionsList> const& definitions = {}
			)
			{
				VertexCommand::CI ci = base;
				derive_command_ci(ci, name, bindings, definitions);
				if (render_pass)
				{
					ci.extern_render_pass = *render_pass;
				}
				return std::make_shared<VertexCommand>(std::move(ci));
			};

			const auto create_derived_compute_command = [&](
				ComputeCommand::CI const& base,
				std::string_view name,
				MyVector<Binding> const& bindings = {},
				Dyn<DefinitionsList> const& definitions = {}
			)
			{
				ComputeCommand::CI ci = base;
				derive_command_ci(ci, name, bindings, definitions);
				return std::make_shared<ComputeCommand>(std::move(ci));
			};

			for (uint32_t model_type : _model_types)
			{
				const VertexCommand::CI base_raster_ci{
					.app = application(),
					.vertex_input_desc = RigidMesh::vertexInputDescFullVertex(),
					.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
					.cull_mode = VK_CULL_MODE_BACK_BIT,
					.sets_layouts = (_sets_layouts + std::pair{model_set, model_layout[model_type]}),
					.write_depth = true,
					.depth_compare_op = VK_COMPARE_OP_LESS,
					.vertex_shader_path = shaders / "RasterGBuffer.vert",
					.fragment_shader_path = shaders / "RasterGBuffer.frag",
				};
				
				auto create_command = [&](std::string_view name, std::shared_ptr<RenderPass> const& render_pass, MyVector<Binding> const& bindings, Dyn<DefinitionsList> const& definitions)
				{
					return create_derived_vertex_command(base_raster_ci, std::format("{}.{}RasterAndShade", this->name(), name), render_pass, bindings, definitions);
				};

				_fat_deferred_pipeline.raster_gbuffer[model_type].raster = create_command("Fat", _fat_deferred_pipeline.render_pass, {}, fat_definitions);

				_minimal_deferred_pipeline.raster_gbuffer[model_type].raster = create_command("Minimal", _minimal_deferred_pipeline.render_pass, {}, minimal_definitions);
			}

			{
				const VertexCommand::CI base_raster_gbuffer_ci{
					.app = application(),
					.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
					.cull_mode = VK_CULL_MODE_BACK_BIT,
					.sets_layouts = _sets_layouts,
					.bindings = {
						Binding{
							.buffer = _model_indices_segment,
							.binding = 0,
						},
					},
					.write_depth = true,
					.depth_compare_op = VK_COMPARE_OP_LESS,
					.vertex_shader_path = shaders / "RasterSceneGBuffer.vert",
					.fragment_shader_path = shaders / "RasterSceneGBuffer.frag",
				};

				auto create_command = [&](std::string_view name, std::shared_ptr<RenderPass> const& render_pass, MyVector<Binding> const& bindings, Dyn<DefinitionsList> const& definitions)
				{
					return create_derived_vertex_command(base_raster_gbuffer_ci, std::format("{}.RasterScene{}GBuffer", this->name(), name), render_pass, bindings, definitions);
				};

				_fat_deferred_pipeline.raster_gbuffer_indirect = create_command("Fat", _fat_deferred_pipeline.render_pass, {}, fat_definitions);

				_minimal_deferred_pipeline.raster_gbuffer_indirect = create_command("Minimal", _minimal_deferred_pipeline.render_pass, {}, minimal_definitions);
			}
			
			_ambient_occlusion = std::make_shared<AmbientOcclusion>(AmbientOcclusion::CI{
				.app = application(),
				.name = name() + ".AO",
				.sets_layouts = _sets_layouts,
				.positions = _fat_deferred_pipeline.position,
				.normals = _fat_deferred_pipeline.normal,
				.can_rt = can_as && (can_rt || can_rq),
			});

			_depth_of_field = std::make_shared<DepthOfField>(DepthOfField::CI{
				.app = application(),
				.name = name() + ".DOF",
				.target = _render_target,
				.depth = _depth,
				.camera = _camera,
				.sets_layouts = _sets_layouts,
			});

			std::shared_ptr<Sampler> bilinear_sampler = application()->getSamplerLibrary().getSampler(SamplerLibrary::SamplerInfo{
				.filter = VK_FILTER_LINEAR,
				.address_mode = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
			});

			{
				ComputeCommand::CI base_shade_from_gbuffer_ci{
					.app = application(),
					.name = name() + ".ShadeFromGBuffer",
					.shader_path = shaders / "ShadeFromGBuffer.comp",
					.extent = _render_target->image()->extent(),
					.dispatch_threads = true,
					.sets_layouts = _sets_layouts,
					.bindings = {
						Binding{
							.image = _render_target,
							.binding = 0,
						},
						Binding{
							.image = _ambient_occlusion->target(),
							.sampler = bilinear_sampler,
							.binding = 1,
						},
					},
					.definitions = [this](DefinitionsList& res)
					{
						res = {
							_use_ao_glsl_def,
							_shadow_method_glsl_def,
						};
					},
				};
				const uint32_t base_shader_binding = base_shade_from_gbuffer_ci.bindings.size32();
				
				auto create_command = [&](std::string_view name, MyVector<Binding> const& bindings, Dyn<DefinitionsList> const& definitions)
				{
					return create_derived_compute_command(base_shade_from_gbuffer_ci, std::format("{}.ShaderFrom{}GBuffer", this->name(), name), bindings, definitions);
				};

				_fat_deferred_pipeline.shade_from_gbuffer = create_command("Fat", 
					{
						Binding{
							.image = _fat_deferred_pipeline.albedo,
							.binding = base_shader_binding + 0,
						},
						Binding{
							.image = _fat_deferred_pipeline.position,
							.binding = base_shader_binding + 1,
						},
						Binding{
							.image = _fat_deferred_pipeline.normal,
							.binding = base_shader_binding + 2,
						},
						Binding{
							.image = _fat_deferred_pipeline.tangent,
							.binding = base_shader_binding + 3,
						},
					},
					fat_definitions
				);

				_minimal_deferred_pipeline.shade_from_gbuffer = create_command("Minimal", 
					{
						Binding{
							.image = _minimal_deferred_pipeline.ids,
							.binding = base_shader_binding + 0,
						},
						Binding{
							.image = _minimal_deferred_pipeline.uvs,
							.binding = base_shader_binding + 1,
						},
					},
					minimal_definitions
				);
			}

			_spot_light_render_pass = std::make_shared<RenderPass>(RenderPass::SPCI{
				.app = application(),
				.name = name() + ".SpotLightRenderPass",
				.depth_stencil = AttachmentDescription2{
					.flags = AttachmentDescription2::Flags::Clear,
					.format = [this]() {return _scene->lightDepthFormat(); },
					.samples = [this]() {return _scene->lightDepthSamples(); },
				},
			});

			_render_spot_light_depth = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RenderSpotLightDepth",
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.cull_mode = VK_CULL_MODE_BACK_BIT, 
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _model_indices_segment,
						.binding = 0,
					},
				},
				.extern_render_pass = _spot_light_render_pass,
				.write_depth = true,
				.depth_compare_op = VK_COMPARE_OP_LESS,
				.vertex_shader_path = shaders / "RasterSceneDepth.vert.slang",
				.fragment_shader_path = shaders / "RasterSceneDepth.frag.slang",
			});

			_point_light_render_pass = std::make_shared<RenderPass>(RenderPass::SPCI{
				.app = application(),
				.name = name() + ".PointLightRenderPass",
				.view_mask = 0b111111,
				.depth_stencil = AttachmentDescription2{
					.flags = AttachmentDescription2::Flags::Clear,
					.format = [this]() {return _scene->lightDepthFormat(); },
					.samples = [this]() {return _scene->lightDepthSamples(); },
				},
			});

			_render_point_light_depth = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RenderPointLightDepth",
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.cull_mode = VK_CULL_MODE_BACK_BIT,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _model_indices_segment,
						.binding = 0,
					},
				},
				.extern_render_pass = _point_light_render_pass,
				.write_depth = true,
				.depth_compare_op = VK_COMPARE_OP_LESS,
				.vertex_shader_path = shaders / "RasterSceneDepth.vert.slang",
				.fragment_shader_path = shaders / "RasterSceneDepth.frag.slang",
				.definitions = Dyn<DefinitionsList>({"TARGET_CUBE 1"}),
			});
		}

		if (application()->availableFeatures().acceleration_structure_khr.accelerationStructure)
		{
			_build_as = std::make_shared<BuildAccelerationStructureCommand>(BuildAccelerationStructureCommand::CI{
				.app = application(),
				.name = name() + ".BuildAS",
			});

			BufferSegment light_transport_ubo = BufferSegment{
				.buffer = _ubo_buffer->buffer(),
				.range = Buffer::Range{.begin = std::alignUp(sizeof(UBO), size_t(1)), .len = std::alignUp(sizeof(LightTransport::UBO), size_t(1))},
			};

			_light_transport = std::make_shared<LightTransport>(LightTransport::CI{
				.app = application(),
				.name = name() + ".LightTransport",
				.scene = _scene,
				.target = _render_target,
				.ubo = light_transport_ubo,
				.sets_layout = _sets_layouts,
			});
		}
	}

	void SimpleRenderer::FillASBuildLists()
	{
		_blas_build_list.clear();
		
		// TODO handle the inheritted visibility
		auto process_mesh = [&](std::shared_ptr<Scene::Node> const& node, Matrix3x4f const& matrix, uint32_t flags)
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
		static thread_local VertexDrawCallInfo _vr;
		VertexDrawCallInfo & vr = _vr;
		vr.clear();
		// TODO handle the inheritted visibility
		auto add_model = [&res, &vr](std::shared_ptr<Scene::Node> const& node, Matrix3x4f const& matrix, uint32_t flags)
		{
			if (node->visible() && node->model() && node->model()->isReadyToDraw())
			{
				vr.clear();
				const uint32_t model_type = node->model()->type();
				node->model()->fillVertexDrawCallInfo(vr);
				
				auto & res_model_type = res[model_type];
				res_model_type.draw_type = DrawType::DrawIndexed;
				res_model_type.pushBack(VertexCommand::DrawCallInfo{
					.name = node->name(),
					.pc_data = &matrix,
					.pc_size = sizeof(matrix),
					.draw_count = vr.draw_count,
					.instance_count = vr.instance_count,
					.index_buffer = vr.index_buffer,
					.index_type = vr.index_type,
					.num_vertex_buffers = vr.vertex_buffers.size32(),
					.vertex_buffers = vr.vertex_buffers.data(),
					.set = node->model()->setAndPool(),
				});
			}
			return node->visible();
		};
		_scene->getTree()->iterateOnDag(add_model);
		vr.clear();
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

	void SimpleRenderer::ForwardPipelineV1::updateResources(UpdateContext& ctx, bool update_direct, bool update_indirect)
	{
		render_pass->updateResources(ctx);
		ctx.resourcesToUpdateLater() += framebuffer;
		if (update_direct)
		{
			for (auto& [m, cmd] : render_scene_direct)
			{
				ctx.resourcesToUpdateLater() += cmd;
			}
		}
		if (update_indirect)
		{
			ctx.resourcesToUpdateLater() += render_scene_indirect;
		}
	}

	void SimpleRenderer::DeferredPipelineBase::updateResources(UpdateContext& ctx, bool update_direct, bool update_indirect)
	{
		render_pass->updateResources(ctx);
		ctx.resourcesToUpdateLater() += framebuffer;
		if (update_direct)
		{
			for (auto& [m, cmd] : raster_gbuffer)
			{
				ctx.resourcesToUpdateLater() += cmd.raster;
			}
		}
		if (update_indirect)
		{
			ctx.resourcesToUpdateLater() += raster_gbuffer_indirect;
		}
		ctx.resourcesToUpdateLater() += shade_from_gbuffer;
	}

	void SimpleRenderer::FatDeferredPipeline::updateResources(UpdateContext& ctx, bool update_direct, bool update_indirect)
	{
		albedo->updateResource(ctx);
		position->updateResource(ctx);
		normal->updateResource(ctx);
		tangent->updateResource(ctx);
		DeferredPipelineBase::updateResources(ctx, update_direct, update_indirect);
	}

	void SimpleRenderer::MinimalDeferredPipeline::updateResources(UpdateContext& ctx, bool update_direct, bool update_indirect)
	{
		ids->updateResource(ctx);
		uvs->updateResource(ctx);
		DeferredPipelineBase::updateResources(ctx, update_direct, update_indirect);
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
			_forward_pipeline.updateResources(ctx, !_use_indirect_rendering || update_all_anyway, _use_indirect_rendering || update_all_anyway);
		}
		if (RenderPipeline(_pipeline_selection.index()) == RenderPipeline::Deferred || update_all_anyway)
		{
			if (_use_fat_gbuffer || update_all_anyway)
			{
				_fat_deferred_pipeline.updateResources(ctx, !_use_indirect_rendering || update_all_anyway, _use_indirect_rendering || update_all_anyway);
			}
			if (!_use_fat_gbuffer || update_all_anyway)
			{
				_minimal_deferred_pipeline.updateResources(ctx, !_use_indirect_rendering || update_all_anyway, _use_indirect_rendering || update_all_anyway);
			}

			_ambient_occlusion->updateResources(ctx);
		}
		if ((RenderPipeline(_pipeline_selection.index()) == RenderPipeline::LightTransport || update_all_anyway) && application()->availableFeatures().acceleration_structure_khr.accelerationStructure)
		{
			_light_transport->updateResources(ctx);
		}
		
		if (_use_indirect_rendering)
		{
			_spot_light_render_pass->updateResources(ctx);
			_point_light_render_pass->updateResources(ctx);
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
								std::shared_ptr<Image> depth_image = lid.depth_view->image();
								my_lid->framebuffer = std::make_shared<Framebuffer>(Framebuffer::CI{
									.app = application(),
									.name = lid.depth_view->name(),
									.render_pass = render_pass,
									.attachments = {lid.depth_view},
									.extent = [depth_image]() {
										return depth_image->extent().value();
									},
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

		_depth_of_field->updateResources(ctx);

		_light_depth_sampler->updateResources(ctx);

		_ubo_buffer->updateResources(ctx);

		_set_layout->updateResources(ctx);
		_set->updateResources(ctx);
	}

	void SimpleRenderer::execute(ExecutionRecorder& exec, float time, float dt, uint32_t frame_id)
	{
		exec.pushDebugLabel(name() + ".execute()", true);

		UBO ubo{
			.time = time,
			.delta_time = dt,
			.frame_idx = frame_id,
			.camera = _camera->getAsGLSL(),
		};
		_ubo_buffer->set(0, &ubo, sizeof(ubo));
		if (RenderPipeline(_pipeline_selection.index()) == RenderPipeline::LightTransport)
		{
			LightTransport::UBO rt_ubo;
			_light_transport->writeUBO(rt_ubo);
			_ubo_buffer->set(sizeof(UBO), &rt_ubo, sizeof(rt_ubo));
		}

		_ubo_buffer->recordTransferIFN(exec);

		const uint32_t set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::module)].set;
		exec.bindSet(BindSetInfo{
			.index = set_index,
			.set = _set,
			.bind_graphics = true,
			.bind_compute = true,
			.bind_rt = true,
		});

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
			const PrepareDrawListPC pc { .num_objects = num_objects };
			exec(_prepare_draw_list->with(ComputeCommand::SingleDispatchInfo{
				.extent = VkExtent3D{.width = num_objects, .height = 1, .depth = 1},
				.dispatch_threads = true,
				.pc_data = &pc,
				.pc_size = sizeof(pc),
			}));
			VertexCommand::DrawInfo& my_draw_list = (_cached_draw_list.begin())->second;
			my_draw_list.draw_type = DrawType::IndirectDraw;
			my_draw_list.pushBack(VertexCommand::DrawCallInfo{
				.draw_count = num_objects,
				.indirect_draw_stride = sizeof(VkDrawIndirectCommand),
				.indirect_draw_buffer = _vk_draw_params_segment,
			});
		}
		else if(generate_host_draw_list)
		{
			tick_tock.tick();
			generateVertexDrawList(draw_list);
			if (exec.framePerfCounters())
			{
				exec.framePerfCounters()->generate_scene_draw_list_time = tick_tock.tockd().count();
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
				VertexCommand::DrawInfo& my_draw_list = (_cached_draw_list.begin())->second;
				const size_t previous_pc_begin = my_draw_list.pc_begin;
				const uint32_t previous_pc_size = my_draw_list.pc_size;
				
				exec.pushDebugLabel("RenderShadowMaps", true);

				RenderPassBeginInfo render_pass;
				VkClearValue clear{
					.depthStencil = {.depth = 1.0f},
				};
				
				for (auto& [path, lid] : _scene->_unique_light_instances)
				{
					std::shared_ptr<Light> const& light = path.path.back()->light();
					LightInstanceData * my_lid = dynamic_cast<LightInstanceData*>(lid.specific_data.get());
					if (light->enableShadowMap() && my_lid && my_lid->framebuffer && ((lid.flags & 1) != 0))
					{
						const uint32_t pc = lid.frame_light_id;
						my_draw_list.setPushConstant(&pc, sizeof(pc));

						render_pass.clear();
						render_pass.ptr_clear_values = &clear;
						render_pass.clear_value_count = 1;
						render_pass.framebuffer = my_lid->framebuffer->instance();

						exec.beginRenderPass(render_pass);

						if (light->type() == LightType::SPOT)
						{
							exec(_render_spot_light_depth->with(my_draw_list));
						}
						else if (light->type() == LightType::POINT)
						{
							exec(_render_point_light_depth->with(my_draw_list));
						}

						exec.endRenderPass();
					}	
				}

				exec.popDebugLabel();
				my_draw_list.pc_begin = previous_pc_begin;
				my_draw_list.pc_size = previous_pc_size;
			}
		}

		if (!draw_list.empty() && _pipeline_selection.index() < 2)
		{
			const RenderPipeline selected_pipeline = static_cast<RenderPipeline>(_pipeline_selection.index());
			if (selected_pipeline == RenderPipeline::Forward)
			{
				tick_tock.tick();
				exec.pushDebugLabel("DirectPipeline", true);
				std::array<VkClearValue, 2> clear_values = {
					VkClearColorValue{.uint32 = {0, 0, 0, 0}},
					VkClearValue{.depthStencil = VkClearDepthStencilValue{.depth = 1.0f}},
				};
				RenderPassBeginInfo render_pass{
					.framebuffer = _forward_pipeline.framebuffer->instance(),
					.clear_value_count = static_cast<uint32_t>(clear_values.size()),
					.ptr_clear_values = clear_values.data(),
				};
				exec.beginRenderPass(render_pass);
				if (_use_indirect_rendering)
				{
					VertexCommand::DrawInfo& my_draw_list = (_cached_draw_list.begin())->second;
					exec(_forward_pipeline.render_scene_indirect->with(my_draw_list));
				}
				else
				{
					for (uint32_t model_type : _model_types)
					{
						if (draw_list[model_type].calls.size())
						{
							exec(_forward_pipeline.render_scene_direct[model_type]->with(draw_list[model_type]));
						}
					}
				}

				exec.endRenderPass();
				
				if (exec.framePerfCounters())
				{
					exec.framePerfCounters()->render_draw_list_time = tick_tock.tockd().count();
				}
				exec.popDebugLabel();
			}
			else if(selected_pipeline == RenderPipeline::Deferred)
			{
				exec.pushDebugLabel("DeferredPipeline", true);
				
				tick_tock.tick();

				DeferredPipelineBase * gbuffer = nullptr;
				if(_use_fat_gbuffer) gbuffer = &_fat_deferred_pipeline;
				else gbuffer = &_minimal_deferred_pipeline;

				VkClearValue clear_depth = VkClearValue{ .depthStencil = VkClearDepthStencilValue{.depth = 1.0f} };
				std::array<VkClearValue, 5> clear_values; 
				uint32_t clear_count;
				if (_use_fat_gbuffer)
				{
					clear_values = {
						VkClearColorValue{.uint32 = {0, 0, 0, 0}},
						VkClearColorValue{.uint32 = {0, 0, 0, 0}},
						VkClearColorValue{.uint32 = {0, 0, 0, 0}},
						VkClearColorValue{.uint32 = {0, 0, 0, 0}},
						clear_depth,
					};
					clear_count = 5;
				}
				else
				{
					clear_values = clear_values = {
						VkClearColorValue{.uint32 = {0, 0, 0, 0}},
						VkClearColorValue{.uint32 = {0, 0, 0, 0}},
						clear_depth,
					};
					clear_count = 3;
				}
				RenderPassBeginInfo render_pass{
					.framebuffer = gbuffer->framebuffer->instance(),
					.clear_value_count = clear_count,
					.ptr_clear_values = clear_values.data(),
				};

				exec.beginRenderPass(render_pass);
				
				if (_use_indirect_rendering)
				{
					VertexCommand::DrawInfo & my_draw_list = (_cached_draw_list.begin())->second;
					{
						exec(gbuffer->raster_gbuffer_indirect->with(my_draw_list));
					}
				}
				else
				{
					for (uint32_t model_type : _model_types)
					{
						if (draw_list[model_type].calls.size())
						{
							exec(gbuffer->raster_gbuffer[model_type].raster->with(draw_list[model_type]));
						}
					}
				}

				exec.endRenderPass();
				
				if (exec.framePerfCounters())
				{
					exec.framePerfCounters()->render_draw_list_time = tick_tock.tockd().count();
				}

				_ambient_occlusion->execute(exec, *_camera);

				exec(gbuffer->shade_from_gbuffer);
				
				exec.popDebugLabel();
			}

			_depth_of_field->record(exec);
		}
		
		if (RenderPipeline(_pipeline_selection.index()) == RenderPipeline::LightTransport)
		{
			_light_transport->render(exec);
		}

		_taau->execute(exec, *_camera);

		{
			// Clear the cached draw list
			for (auto& [vt, vl] : _cached_draw_list)
			{
				vl.clear();
			}
		}

		exec.bindSet(BindSetInfo{
			.index = set_index,
			.set = nullptr,
			.bind_graphics = true,
			.bind_compute = true,
			.bind_rt = true,
		});

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

			RenderPipeline render_pipeline = RenderPipeline(_pipeline_selection.index());

			if (render_pipeline == RenderPipeline::Deferred)
			{
				if (ImGui::CollapsingHeader(_ambient_occlusion->name().c_str()))
				{
					_ambient_occlusion->declareGui(ctx);
					ImGui::Separator();
				}
			}
			else if (render_pipeline == RenderPipeline::LightTransport)
			{
				_light_transport->declareGUI(ctx);
			}
			
			ImGui::BeginDisabled(true);
			ImVec4 color;
			color = _maintain_rt ? ctx.style().valid_green : ctx.style().invalid_red;
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

			if (ImGui::CollapsingHeader("Depth Of Field"))
			{
				_depth_of_field->declareGUI(ctx);
				ImGui::Separator();
			}
		}
		ImGui::PopID();
	}
}