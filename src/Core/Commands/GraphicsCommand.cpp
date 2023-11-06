#include "GraphicsCommand.hpp"

namespace vkl
{
	GraphicsCommand::GraphicsCommand(CreateInfo const& ci) :
		ShaderCommand(ShaderCommand::CreateInfo{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts,
		}),
		_topology(ci.topology),
		_vertex_input_desc(ci.vertex_input_description),
		_attachements(ci.targets),
		_depth(ci.depth_buffer),
		_write_depth(ci.write_depth),
		_clear_color(ci.clear_color),
		_clear_depth_stencil(ci.clear_depth_stencil),
		_blending(ci.blending),
		_line_raster_mode(ci.line_raster_mode),
		_draw_type(ci.draw_type)
	{}

	void GraphicsCommand::createGraphicsResources()
	{
		using namespace std::containers_operators;
		const uint32_t n_color = static_cast<uint32_t>(_attachements.size());
		std::vector<VkAttachmentDescription2> at_desc(n_color);
		std::vector<VkAttachmentReference2> at_ref(n_color);
		for (size_t i = 0; i < n_color; ++i)
		{
			at_desc[i] = VkAttachmentDescription2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.pNext = nullptr,
				.flags = 0,
				.format = _attachements[i]->format(),
				.samples = _attachements[i]->image()->sampleCount(),
				.loadOp = _clear_color.has_value() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};
			at_ref[i] = VkAttachmentReference2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
				.pNext = nullptr,
				.attachment = uint32_t(i),
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};
		}


		const bool render_depth = !!_depth;

		if (render_depth)
		{
			const bool write_depth = _write_depth.value_or(true);

			at_desc.push_back(VkAttachmentDescription2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.pNext = nullptr,
				.flags = 0,
				.format = _depth->format(),
				.samples = _depth->image()->sampleCount(),
				.loadOp = _clear_depth_stencil.has_value() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = write_depth ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_NONE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			});
			at_ref.push_back(VkAttachmentReference2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
				.pNext = nullptr,
				.attachment = static_cast<uint32_t>(at_desc.size() - 1),
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			});
		}

		VkSubpassDescription2 subpass = {
			.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
			.pNext = nullptr,
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = n_color,
			.pColorAttachments= at_ref.data(), // Warning this is unsafe, the ptr is copied, assuming the data is copied later
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = render_depth ? (at_ref.data() + n_color) : nullptr, // Warning this is unsafe, the ptr is copied, assuming the data is copied later
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr,
		};
		std::vector<VkSubpassDependency2> dependencies;

		VkSubpassDependency2 color_dependency = {
			.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
			.pNext = nullptr,
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_NONE,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		};
		dependencies.push_back(color_dependency);

		if (render_depth)
		{
			VkSubpassDependency2 depth_dependency = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
				.pNext = nullptr,
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				.srcAccessMask = VK_ACCESS_NONE,
				.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, // TODO if read depth
			};
			dependencies.push_back(depth_dependency);
		}

		_render_pass = std::make_shared <RenderPass>(RenderPass::CI{
			.app = application(),
			.name = name() + ".RenderPass",
			.attachement_descriptors = at_desc,
			.attachement_ref_per_subpass = {at_ref},
			.subpasses = {subpass},
			.dependencies = dependencies,
			.last_is_depth = render_depth,
		});

		_framebuffer = std::make_shared<Framebuffer>(Framebuffer::CI{
			.app = application(),
			.name = name() + ".Framebuffer",
			.render_pass = _render_pass,
			.targets = _attachements,
			.depth = _depth,
		});
	}

	void GraphicsCommand::declareGraphicsResources(SynchronizationHelper & synch)
	{
		for (size_t i = 0; i < _framebuffer->size(); ++i)
		{
			std::shared_ptr<ImageView> view = _framebuffer->textures()[i];
			synch.addSynch(Resource{
				._image = view,
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, // TODO add read bit if alpha blending 
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				},
				._image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			});
		}
		if (!!_depth)
		{
			const VkAccessFlags2 access2 = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT; // TODO deduce from the depth test;
			synch.addSynch(Resource{
				._image = _depth,
				._begin_state = ResourceState2{
					.access = access2,
					.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, // TODO deduce from fragment shader reflection
				},
				._end_state = ResourceState2{
					.access = access2,
					.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, // TODO deduce from fragment shader reflection
				},
				._image_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			});
		}
	}

	void GraphicsCommand::createPipeline()
	{
		Pipeline::GraphicsCreateInfo gci;
		gci.app = application();
		gci.name = name() + ".Pipeline";
		gci.vertex_input = _vertex_input_desc;
		gci.input_assembly = Pipeline::InputAssemblyDefault(_topology);
		gci.rasterization = Pipeline::RasterizationDefault(VK_CULL_MODE_BACK_BIT);
		
		if (_line_raster_mode.has_value())
		{
			gci.line_raster = Pipeline::LineRasterization(_line_raster_mode.value());
		}

		gci.multisampling = Pipeline::MultisampleOneSample();
		gci.program = _program;
		gci.render_pass = _render_pass;

		if (_depth)
		{
			gci.depth_stencil = Pipeline::DepthStencilCloser(_write_depth.value_or(true));
		}

		if (false)
		{
			VkViewport viewport = Pipeline::Viewport(extract(*_framebuffer->extent()));
			VkRect2D scissor = Pipeline::Scissor(extract(*_framebuffer->extent()));

			gci.viewports = { viewport };
			gci.scissors = { scissor };
		}
		else
		{
			gci.dynamic = { VK_DYNAMIC_STATE_VIEWPORT , VK_DYNAMIC_STATE_SCISSOR };
		}
		std::vector<VkPipelineColorBlendAttachmentState> blending(_framebuffer->size(), _blending.value_or(Pipeline::BlendAttachementNoBlending()));
		gci.attachements_blends = blending;


		_pipeline = std::make_shared<Pipeline>(gci);
	}

	void GraphicsCommand::init()
	{
		ShaderCommand::init();
	}

	bool GraphicsCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= ShaderCommand::updateResources(ctx);

		_framebuffer->updateResources(ctx);

		return res;
	}

	void GraphicsCommand::recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context, DrawInfo const& di, void * user_info)
	{
		context.pushDebugLabel(name());
		SynchronizationHelper synch(context);

		// Synch framebuffer
		declareGraphicsResources(synch);

		// Bind descriptor sets up to shader
		recordBindings(cmd, context);
		// Synch bound resources up to descriptor set
		recordBoundResourcesSynchronization(context.graphicsBoundSets(), synch, application()->descriptorBindingGlobalOptions().shader_set + 1);
		
		// Synch models to draw
		synchronizeDrawResources(synch, user_info);

		
		synch.record();

		VkExtent2D render_area = extract(*_framebuffer->extent());

		const size_t num_clear_values = (_clear_color.has_value() || _clear_depth_stencil.has_value()) ? (_framebuffer->size() + 1) : 0;

		std::vector<VkClearValue> clear_values(num_clear_values);
		if (num_clear_values)
		{
			if (_clear_color.has_value())
			{
				const VkClearValue cv = { .color = _clear_color.value() };
				std::fill_n(clear_values.begin(), _framebuffer->size(), cv);
			}
			if (_clear_depth_stencil.has_value())
			{
				clear_values.back() = VkClearValue{ .depthStencil = _clear_depth_stencil.value() };
			}
		}

		const std::string pass_name = name();
		
		VkRenderPassBeginInfo begin = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = *_render_pass,
			.framebuffer = *_framebuffer->instance(),
			.renderArea = VkRect2D{.offset = makeZeroOffset2D(), .extent = render_area},
			.clearValueCount = static_cast<uint32_t>(num_clear_values),
			.pClearValues = num_clear_values ? clear_values.data() : nullptr,
		};

		vkCmdBeginRenderPass(cmd, &begin, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *_pipeline->instance());
			if (false)
			{

			}
			else
			{
				VkViewport viewport = Pipeline::Viewport(extract(*_framebuffer->extent()));
				VkRect2D scissor = Pipeline::Scissor(extract(*_framebuffer->extent()));
				vkCmdSetViewport(cmd, 0, 1, &viewport);
				vkCmdSetScissor(cmd, 0, 1, &scissor);
			}
			recordBindings(cmd, context);
			recordDraw(cmd, context, user_info);
		}
		vkCmdEndRenderPass(cmd);

		context.keppAlive(_pipeline->instance());
		context.keppAlive(_framebuffer->instance());
		context.keppAlive(_render_pass);
		context.popDebugLabel();
	}

	VertexCommand::VertexCommand(CreateInfo const& ci) :
		GraphicsCommand(GraphicsCommand::CreateInfo{
			.app = ci.app,
			.name = ci.name,
			.topology = ci.topology,
			.vertex_input_description = ci.vertex_input_desc,
			.line_raster_mode = ci.line_raster_mode,
			.sets_layouts = ci.sets_layouts,
			.bindings = ci.bindings,
			.targets = ci.color_attachements,
			.depth_buffer = ci.depth_buffer,
			.write_depth = ci.write_depth,
			.clear_color = ci.clear_color,
			.clear_depth_stencil = ci.clear_depth_stencil,
			.blending = ci.blending,
			.draw_type = ci.draw_type,
		}),
		_shaders(ShaderPaths{
			.vertex_path = ci.vertex_shader_path, 
			.tess_control_path = ci.tess_control_shader_path,
			.tess_eval_path = ci.tess_eval_shader_path,
			.geometry_path = ci.geometry_shader_path, 
			.fragment_path = ci.fragment_shader_path, 
			.definitions = ci.definitions 
		}),
		_draw_count(ci.draw_count)
	{
		createProgram();
		createGraphicsResources();
		createPipeline();

		_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CI{
			.app = application(),
			.name = name() + ".set",
			.program = _program,
			.target_set = application()->descriptorBindingGlobalOptions().shader_set,
			.bindings = ci.bindings,
		});
	}

	void VertexCommand::createProgram()
	{
		std::shared_ptr<Shader> vert = nullptr, tess_control = nullptr, tess_eval = nullptr, geom = nullptr, frag = nullptr;
		if (!_shaders.vertex_path.empty())
		{
			vert = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".vert",
				.source_path = _shaders.vertex_path,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.definitions = _shaders.definitions
			});
		}
		if (!_shaders.tess_control_path.empty())
		{
			tess_control = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".tess_control",
				.source_path = _shaders.tess_control_path,
				.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
				.definitions = _shaders.definitions
			});
		}
		if (!_shaders.tess_eval_path.empty())
		{
			tess_eval = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".tess_eval",
				.source_path = _shaders.tess_eval_path,
				.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
				.definitions = _shaders.definitions
			});
		}
		if (!_shaders.geometry_path.empty())
		{
			geom = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".geom",
				.source_path = _shaders.geometry_path,
				.stage = VK_SHADER_STAGE_GEOMETRY_BIT,
				.definitions = _shaders.definitions
			});
		}
		if (!_shaders.fragment_path.empty())
		{
			frag = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".frag",
				.source_path = _shaders.fragment_path,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.definitions = _shaders.definitions
			});
		}
		_program = std::make_shared<GraphicsProgram>(GraphicsProgram::CreateInfoVertex{
			.app = application(),
			.name = name() + ".Program",
			.sets_layouts = _provided_sets_layouts,
			.vertex = vert,
			.tess_control = tess_control,
			.tess_eval = tess_eval,
			.geometry = geom,
			.fragment = frag,
		});
	}

	void VertexCommand::synchronizeDrawResources(SynchronizationHelper& synch, void* user_data)
	{
		DrawInfo const& di = *reinterpret_cast<DrawInfo*>(user_data);

		std::shared_ptr<DescriptorSetLayout> layout = [&]() -> std::shared_ptr<DescriptorSetLayout> {
			const auto & layouts = _program->instance()->reflectionSetsLayouts();
			const uint32_t set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::object)].set;
			if (set_index < layouts.size())
			{
				return layouts[set_index];
			}
			else
			{
				return nullptr;
			}
		}();
			
		// Synchronize for vertex attribute fetch and descriptor binding
		std::set<void*> already_sync;
		for (auto& to_draw : di.draw_list)
		{
			if(layout && !already_sync.contains(to_draw.set.get()))
			{
				assert(!!to_draw.set);
				recordDescriptorSetSynch(synch, *to_draw.set->instance(), *layout);
				already_sync.emplace(to_draw.set.get());
			}

			// Hack solution to not trigger a synch validation error -> will have to rewrite the synch helper soon 
			//if (false && to_draw.vertex_buffers.size() == 1 && to_draw.index_buffer == to_draw.vertex_buffers[0].buffer)
			//{
			//	Buffer::Range range;
			//	range.begin = std::min(to_draw.index_buffer_range.begin, to_draw.vertex_buffers[0].range.begin);
			//	size_t end = std::max(to_draw.index_buffer_range.end(), to_draw.vertex_buffers[0].range.end());
			//	range.len = end - range.begin;
			//	synch.addSynch(Resource{
			//		._buffer = to_draw.index_buffer,
			//		._buffer_range = range,
			//		._begin_state = ResourceState2{
			//			.access = VK_ACCESS_2_INDEX_READ_BIT | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
			//			.stage = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
			//		},
			//	});
			//}
			//else
			{
				if (to_draw.index_buffer)
				{
					synch.addSynch(Resource{
						._buffer = to_draw.index_buffer,
						._buffer_range = to_draw.index_buffer_range,
						._begin_state = ResourceState2{
							.access = VK_ACCESS_2_INDEX_READ_BIT,
							.stage = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
						},
					});
				}
			
				for (VertexBuffer vb : to_draw.vertex_buffers)
				{
					synch.addSynch(Resource{
						._buffer = vb.buffer,
						._buffer_range = vb.range,
						._begin_state = ResourceState2{
							.access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
							.stage = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
						},
					});
				}
			}
		}
	}

	void VertexCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context, void * user_data)
	{
		DrawInfo const& di = *reinterpret_cast<DrawInfo*>(user_data);
		
		const uint32_t set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::object)].set;
		const std::shared_ptr<PipelineLayout> & layout = _pipeline->program()->instance()->pipelineLayout();
		std::vector<VkBuffer> vb_bind;
		std::vector<VkDeviceSize> vb_offsets;
		for (auto& to_draw : di.draw_list)
		{
			if (!to_draw.name.empty())
			{
				context.pushDebugLabel(to_draw.name);
			}
			std::shared_ptr<DescriptorSetAndPool> set_desc = to_draw.set;
			if(set_desc)
			{
				std::shared_ptr<DescriptorSetAndPoolInstance> set = set_desc->instance();
				if (set->exists() && !set->empty())
				{
					context.graphicsBoundSets().bindOneAndRecord(set_index, set, layout);
					context.keppAlive(set);
				}
			}
			recordPushConstant(cmd, context, to_draw.pc);
			if (to_draw.index_buffer)
			{
				vkCmdBindIndexBuffer(cmd, to_draw.index_buffer->instance()->handle(), to_draw.index_buffer_range.begin, to_draw.index_type);
				context.keppAlive(to_draw.index_buffer->instance());
			}
			if (!to_draw.vertex_buffers.empty())
			{
				vb_bind.resize(to_draw.vertex_buffers.size());
				vb_offsets.resize(to_draw.vertex_buffers.size());
				for (size_t i = 0; i < vb_bind.size(); ++i)
				{
					vb_bind[i] = to_draw.vertex_buffers[i].buffer->instance()->handle();
					vb_offsets[i] = to_draw.vertex_buffers[i].range.begin;
					context.keppAlive(to_draw.vertex_buffers[i].buffer->instance());
				}
				vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32_t>(to_draw.vertex_buffers.size()), vb_bind.data(), vb_offsets.data());
			}

			switch (di.draw_type)
			{
				case DrawType::Draw:
					vkCmdDraw(cmd, to_draw.draw_count, to_draw.instance_count, 0, 0);
				break;
				case DrawType::DrawIndexed:
					vkCmdDrawIndexed(cmd, to_draw.draw_count, to_draw.instance_count, 0, 0, 0);
				break;
				default:
					assertm(false, "Unsupported draw call type");
				break;
			}
			if (!to_draw.name.empty())
			{
				context.popDebugLabel();
			}
		}
	}

	void VertexCommand::execute(ExecutionContext & ctx)
	{
		CommandBuffer& cmd = *ctx.getCommandBuffer();

		NOT_YET_IMPLEMENTED;

		GraphicsCommand::DrawInfo gdi{
			
		};

		DrawInfo di{
			
		};

		recordCommandBuffer(cmd, ctx, gdi, &di);
	}

	Executable VertexCommand::with(DrawInfo const& di)
	{
		GraphicsCommand::DrawInfo gdi{
			
		};

		DrawInfo _di = di;

		return [this, gdi, _di](ExecutionContext& ctx)
		{
			recordCommandBuffer(*ctx.getCommandBuffer(), ctx, gdi, (void*) & _di);
		};
	}

	bool VertexCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= GraphicsCommand::updateResources(ctx);

		return res;
	}





	MeshCommand::MeshCommand(CreateInfo const& ci) :
		GraphicsCommand(GraphicsCommand::CreateInfo{
			.app = ci.app,
			.name = ci.name,
			.line_raster_mode = ci.line_raster_mode,
			.sets_layouts = ci.sets_layouts,
			.bindings = ci.bindings,
			.targets = ci.color_attachements,
			.depth_buffer = ci.depth_buffer,
			.write_depth = ci.write_depth,
			.clear_color = ci.clear_color,
			.clear_depth_stencil = ci.clear_depth_stencil,
			.blending = ci.blending,
		}),
		_shaders{
			.task_path = ci.task_shader_path,
			.mesh_path = ci.mesh_shader_path,
			.fragment_path = ci.fragment_shader_path,
			.definitions = ci.definitions,
		},
		_extent(ci.extent),
		_dispatch_threads(ci.dispatch_threads)
	{
		createProgram();
		createGraphicsResources();
		createPipeline();

		_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CI{
			.app = application(),
				.name = name() + ".set",
				.program = _program,
				.target_set = application()->descriptorBindingGlobalOptions().shader_set,
				.bindings = ci.bindings,
		});
	}

	void MeshCommand::createProgram()
	{
		std::shared_ptr<Shader> task = nullptr, mesh = nullptr, frag = nullptr;
		if (!_shaders.task_path.empty())
		{
			task = std::make_shared<Shader>(Shader::CI{
				.app = application(),
					.name = name() + ".task",
					.source_path = _shaders.task_path,
					.stage = VK_SHADER_STAGE_TASK_BIT_EXT,
					.definitions = _shaders.definitions
			});
		}
		if (!_shaders.mesh_path.empty())
		{
			mesh = std::make_shared<Shader>(Shader::CI{
				.app = application(),
					.name = name() + ".mesh",
					.source_path = _shaders.mesh_path,
					.stage = VK_SHADER_STAGE_MESH_BIT_EXT,
					.definitions = _shaders.definitions
			});
		}
		if (!_shaders.fragment_path.empty())
		{
			frag = std::make_shared<Shader>(Shader::CI{
				.app = application(),
					.name = name() + ".frag",
					.source_path = _shaders.fragment_path,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.definitions = _shaders.definitions
			});
		}

		_program = std::make_shared<GraphicsProgram>(GraphicsProgram::CreateInfoMesh{
			.app = application(),
			.name = name() + ".Program",
			.sets_layouts = _provided_sets_layouts,
			.task = task,
			.mesh = mesh,
			.fragment = frag,
		});
	}



	void MeshCommand::synchronizeDrawResources(SynchronizationHelper& synch, void* user_data)
	{
		DrawInfo const& di = *reinterpret_cast<DrawInfo*>(user_data);

		std::shared_ptr<DescriptorSetLayout> layout = [&]() -> std::shared_ptr<DescriptorSetLayout> {
			const auto& layouts = _program->instance()->reflectionSetsLayouts();
			const uint32_t set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::object)].set;
			if (set_index < layouts.size())
			{
				return layouts[set_index];
			}
			else
			{
				return nullptr;
			}
		}();

		if (layout)
		{
			std::set<void*> already_sync;
			for (auto& to_draw : di.draw_list)
			{
				if (!already_sync.contains(to_draw.set.get()))
				{
					assert(!!to_draw.set);
					recordDescriptorSetSynch(synch, *to_draw.set->instance(), *layout);
					already_sync.emplace(to_draw.set.get());
				}
			}
		}
	}


	void MeshCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context, void* user_data)
	{
		assert(application()->availableFeatures().mesh_shader_ext.meshShader);
		DrawInfo const& di = *reinterpret_cast<DrawInfo*>(user_data);

		const auto & _vkCmdDrawMeshTasksEXT = application()->extFunctions()._vkCmdDrawMeshTasksEXT;

		const uint32_t set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::object)].set;
		const std::shared_ptr<PipelineLayout>& layout = _pipeline->program()->instance()->pipelineLayout();
		
		for (auto& to_draw : di.draw_list)
		{
			const VkExtent3D work = di.dispatch_threads ? getWorkgroupsDispatchSize(to_draw.extent) : to_draw.extent;
			
			std::shared_ptr<DescriptorSetAndPool> set_desc = to_draw.set;
			if (set_desc)
			{
				std::shared_ptr<DescriptorSetAndPoolInstance> set = set_desc->instance();
				if (set->exists() && !set->empty())
				{
					context.graphicsBoundSets().bindOneAndRecord(set_index, set, layout);
					context.keppAlive(set);
				}
			}
			recordPushConstant(cmd, context, to_draw.pc);

			switch (di.draw_type)
			{
				case DrawType::Dispatch:
					_vkCmdDrawMeshTasksEXT(cmd, work.width, work.height, work.depth);
				break;
				default:
					assertm(false, "Unsupported draw call type");
				break;
			}
		}
	}

	void MeshCommand::execute(ExecutionContext& ctx)
	{
		CommandBuffer& cmd = *ctx.getCommandBuffer();

		GraphicsCommand::DrawInfo gdi{
			
		};

		NOT_YET_IMPLEMENTED;

		DrawInfo di{
			.dispatch_threads = _dispatch_threads,
		};

		recordCommandBuffer(cmd, ctx, gdi, &di);
	}

	Executable MeshCommand::with(DrawInfo const& di)
	{
		GraphicsCommand::DrawInfo gdi{

		};

		DrawInfo _di = di;

		return [this, gdi, _di](ExecutionContext& ctx)
		{
			recordCommandBuffer(*ctx.getCommandBuffer(), ctx, gdi, (void*)&_di);
		};
	}

	bool MeshCommand::updateResources(UpdateContext& ctx)
	{
		bool res = false;

		res |= GraphicsCommand::updateResources(ctx);

		return res;
	}


	//void FragCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	//{
	//	vkCmdDrawIndexed(cmd, 3, 1, 0, 0, 0);
	//}
}