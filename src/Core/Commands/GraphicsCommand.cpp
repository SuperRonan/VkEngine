#include "GraphicsCommand.hpp"

namespace vkl
{
	GraphicsCommand::GraphicsCommand(CreateInfo const& ci) :
		ShaderCommand(ci.app, ci.name, ci.bindings),
		_topology(ci.topology),
		_vertex_input_desc(ci.vertex_input_description),
		_attachements(ci.targets),
		_depth(ci.depth_buffer),
		_write_depth(ci.write_depth),
		_clear_color(ci.clear_color),
		_clear_depth_stencil(ci.clear_depth_stencil),
		_blending(ci.blending),
		_line_raster_mode(ci.line_raster_mode)
	{}

	void GraphicsCommand::createGraphicsResources()
	{
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
			.pColorAttachments= at_ref.data(), // Warning this is unsafe, the data is copied
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = render_depth ? (at_ref.data() + n_color) : nullptr,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr,
		};
		VkSubpassDependency2 dependency = {
			.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
			.pNext = nullptr,
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_NONE,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};
		
		//if (render_depth)
		//{
		//	dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		//}

		_render_pass = std::make_shared <RenderPass>(RenderPass::CI{
			.app = application(),
			.name = name() + ".RenderPass",
			.attachement_descriptors = at_desc,
			.attachement_ref_per_subpass = {at_ref},
			.subpasses = {subpass},
			.dependencies = {dependency},
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
					.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, // TODO add read bit if alpha blending 
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
		SynchronizationHelper synch(context);

		declareGraphicsResources(synch);
		_sets->instance()->recordInputSynchronization(synch);

		VkExtent2D render_area = extract(*_framebuffer->extent());

		const size_t num_clear_values = (_clear_color.has_value() || _clear_depth_stencil.has_value()) ? (_framebuffer->size() + 1) : 0;

		std::vector<VkClearValue> clear_values(num_clear_values);
		if (num_clear_values)
		{
			if (_clear_color.has_value())
			{
				const VkClearValue cv = { .color = _clear_color.value() };
				std::fill_n(clear_values.begin(), num_clear_values - 1, cv);
			}
			if (_clear_depth_stencil.has_value())
			{
				clear_values.back() = VkClearValue{ .depthStencil = _clear_depth_stencil.value() };
			}
		}

		VkRenderPassBeginInfo begin = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = *_render_pass,
			.framebuffer = *_framebuffer->instance(),
			.renderArea = VkRect2D{.offset = makeZeroOffset2D(), .extent = render_area},
			.clearValueCount = static_cast<uint32_t>(num_clear_values),
			.pClearValues = num_clear_values ? clear_values.data() : nullptr,
		};

		synch.record();

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
			recordPushConstant(cmd, context, di.pc);
			recordDraw(cmd, context, user_info);
		}
		vkCmdEndRenderPass(cmd);

		context.keppAlive(_pipeline->instance());
		context.keppAlive(_framebuffer->instance());
		context.keppAlive(_render_pass);
		context.keppAlive(_sets->instance());
	}

	VertexCommand::VertexCommand(CreateInfo const& ci) :
		GraphicsCommand(GraphicsCommand::CreateInfo{
			.app = ci.app,
			.name = ci.name,
			.topology = ci.topology,
			.vertex_input_description = ci.vertex_input_desc,
			.line_raster_mode = ci.line_raster_mode,
			.bindings = ci.bindings,
			.targets = ci.color_attachements,
			.depth_buffer = ci.depth_buffer,
			.write_depth = ci.write_depth,
			.clear_color = ci.clear_color,
			.clear_depth_stencil = ci.clear_depth_stencil,
			.blending = ci.blending,
		}),
		_shaders(ShaderPaths{
			.vertex_path = ci.vertex_shader_path, 
			.tess_control_path = ci.tess_control_shader_path,
			.tess_eval_path = ci.tess_eval_shader_path,
			.geometry_path = ci.geometry_shader_path, 
			.fragment_path = ci.fragment_shader_path, 
			.definitions = ci.definitions 
		}),
		_draw_count(ci.draw_count),
		_meshes(ci.meshes)
	{
		createProgram();
		createGraphicsResources();
		createPipeline();

		_sets = std::make_shared<DescriptorSetsManager>(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + ".sets",
			.bindings = ci.bindings,
			.program = _program,
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
			.vertex = vert,
			.tess_control = tess_control,
			.tess_eval = tess_eval,
			.geometry = geom,
			.fragment = frag,
		});
	}

	void VertexCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context, void * user_data)
	{
		DrawInfo const& di = *reinterpret_cast<DrawInfo*>(user_data);
		if (di.draw_count > 0)
		{
			vkCmdDraw(cmd, di.draw_count, 1, 0, 0);
		}
		else
		{
			for (auto& mesh : di.meshes)
			{
				recordPushConstant(cmd, context, mesh.pc);
				mesh.mesh->recordBindAndDraw(cmd);
			}
		}
	}

	void VertexCommand::execute(ExecutionContext & ctx)
	{
		CommandBuffer& cmd = *ctx.getCommandBuffer();

		GraphicsCommand::DrawInfo gdi{
			.pc = _pc,
		};

		DrawInfo di{
			.draw_count = _draw_count.valueOr(0),
		};

		recordCommandBuffer(cmd, ctx, gdi, &di);
	}

	Executable VertexCommand::with(DrawInfo const& di)
	{
		GraphicsCommand::DrawInfo gdi{
			.pc = di.pc.hasValue() ? di.pc : _pc,
		};

		DrawInfo _di{
			.draw_count = di.draw_count ? di.draw_count : (_draw_count.hasValue() ? _draw_count.value() : 0),
			.meshes = di.meshes,
		};

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
		_dispatch_size(ci.dispatch_size),
		_dispatch_threads(ci.dispatch_threads)
	{
		createProgram();
		createGraphicsResources();
		createPipeline();

		_sets = std::make_shared<DescriptorSetsManager>(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + ".sets",
			.bindings = ci.bindings,
			.program = _program,
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
			.task = task,
			.mesh = mesh,
			.fragment = frag,
		});
	}


	void MeshCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context, void* user_data)
	{
		assert(application()->availableFeatures().mesh_shader_ext.meshShader);
		DrawInfo const& di = *reinterpret_cast<DrawInfo*>(user_data);
		{
			const VkExtent3D work = di.dispatch_threads.value() ? getWorkgroupsDispatchSize(*di.dispatch_size) : *di.dispatch_size;
			const auto & _vkCmdDrawMeshTasksEXT = application()->extFunctions()._vkCmdDrawMeshTasksEXT;
			_vkCmdDrawMeshTasksEXT(cmd, work.width, work.height, work.depth);
		}
	}

	void MeshCommand::execute(ExecutionContext& ctx)
	{
		CommandBuffer& cmd = *ctx.getCommandBuffer();

		GraphicsCommand::DrawInfo gdi{
			.pc = _pc,
		};

		DrawInfo di{
			.dispatch_size = *_dispatch_size,
			.dispatch_threads = _dispatch_threads,
		};

		recordCommandBuffer(cmd, ctx, gdi, &di);
	}

	Executable MeshCommand::with(DrawInfo const& di)
	{
		GraphicsCommand::DrawInfo gdi{
			.pc = di.pc.hasValue() ? di.pc : _pc,
		};

		DrawInfo _di{
			.dispatch_size = di.dispatch_size.has_value() ? di.dispatch_size : *_dispatch_size,
			.dispatch_threads = di.dispatch_threads.value_or(_dispatch_threads),
		};

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