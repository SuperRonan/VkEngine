#include "Pipeline.hpp"

namespace vkl
{

	const VkGraphicsPipelineCreateInfo& PipelineInstance::GraphicsCreateInfo::assemble() const
	{
		{
			if (vertex_input.has_value())
				_vk_vertex_input = vertex_input->link();

			uint32_t num_viewport = static_cast<uint32_t>(viewports.size());
			uint32_t num_scissor = static_cast<uint32_t>(scissors.size());
			if (std::find(dynamic.cbegin(), dynamic.cend(), VK_DYNAMIC_STATE_VIEWPORT) != dynamic.cend())
			{
				num_viewport = 1;
			}
			if (std::find(dynamic.cbegin(), dynamic.cend(), VK_DYNAMIC_STATE_SCISSOR) != dynamic.cend())
			{
				num_scissor = 1;
			}

			_viewport = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
				.viewportCount = num_viewport,
				.pViewports = viewports.data(),
				.scissorCount = num_scissor,
				.pScissors = scissors.data(),
			};

			_blending = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				.logicOpEnable = VK_FALSE, // TODO
				.attachmentCount = (uint32_t)attachements_blends.size(),
				.pAttachments = attachements_blends.data(),
				.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
			};

			_shaders.resize(program->shaders().size());
			for (size_t i = 0; i < _shaders.size(); ++i)
			{
				_shaders[i] = program->shaders()[i]->getPipelineShaderStageCreateInfo();
			}

			_dynamic_state = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.dynamicStateCount = static_cast<uint32_t>(dynamic.size()),
				.pDynamicStates = dynamic.data(),
			};

			if (line_raster.has_value() && app->hasDeviceExtension(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME))
			{
				rasterization.pNext = &line_raster.value();
			}
			else
			{
				rasterization.pNext = nullptr;
			}
		}


		_pipeline_ci = VkGraphicsPipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = (uint32_t)_shaders.size(),
			.pStages = _shaders.data(),
			.pVertexInputState = vertex_input.has_value() ? &_vk_vertex_input : nullptr,
			.pInputAssemblyState = input_assembly.has_value() ? &input_assembly.value() : nullptr,
			.pViewportState = &_viewport,
			.pRasterizationState = &rasterization,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = depth_stencil.has_value() ? &depth_stencil.value() : nullptr,
			.pColorBlendState = &_blending,
			.pDynamicState = &_dynamic_state,
			.layout = *program->pipelineLayout(),
			.renderPass = *render_pass->instance(),
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0,
		};
		return _pipeline_ci;
	}

	PipelineInstance::~PipelineInstance()
	{
		if (_handle != VK_NULL_HANDLE)
		{
			callDestructionCallbacks();
			vkDestroyPipeline(_app->device(), _handle, nullptr);
			_handle = VK_NULL_HANDLE;
		}
	}

	PipelineInstance::PipelineInstance(GraphicsCreateInfo const& gci) :
		AbstractInstance(gci.app, gci.name),
		_binding(VK_PIPELINE_BIND_POINT_GRAPHICS),
		_program(gci.program),
		_layout(gci.program->pipelineLayout()),
		_render_pass(gci.render_pass)
	{

		VK_CHECK(vkCreateGraphicsPipelines(_app->device(), nullptr, 1, &gci.assemble(), nullptr, &_handle), "Failed to create a graphics pipeline.");
	}

	PipelineInstance::PipelineInstance(ComputeCreateInfo const& cci) :
		AbstractInstance(cci.app, cci.name),
		_binding(VK_PIPELINE_BIND_POINT_COMPUTE),
		_program(cci.program),
		_layout(cci.program->pipelineLayout())
	{
		const VkComputePipelineCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = cci.program->shader()->getPipelineShaderStageCreateInfo(),
			.layout = *_program->pipelineLayout(),
		};
		VK_CHECK(vkCreateComputePipelines(_app->device(), nullptr, 1, &ci, nullptr, &_handle), "Failed to create a compute pipeline.");
	}

	void Pipeline::createInstance()
	{
		waitForInstanceCreationIFN();

		std::vector<std::shared_ptr<AsynchTask>> dependecies;
		if (_program->creationTask())
		{
			dependecies.push_back(_program->creationTask());
		}

		_create_instance_task = std::make_shared<AsynchTask>(AsynchTask::CI{
			.name = "Create Pipeline " + name(),
			.priority = TaskPriority::ASAP(),
			.lambda = [this]() {
		
				if (_binding == VK_PIPELINE_BIND_POINT_GRAPHICS)
				{
					PipelineInstance::GraphicsCreateInfo gci;
					gci.app = application();
					gci.name = name();
					gci.vertex_input = _gci.vertex_input;
					gci.input_assembly = _gci.input_assembly;
					gci.viewports = _gci.viewports;
					gci.scissors = _gci.scissors;
					gci.rasterization = _gci.rasterization;
					gci.line_raster = _gci.line_raster;
					gci.multisampling = _gci.multisampling;
					gci.depth_stencil = _gci.depth_stencil;
					gci.attachements_blends = _gci.attachements_blends;
					gci.render_pass = _gci.render_pass;
					gci.dynamic = _gci.dynamic;
					gci.program = std::dynamic_pointer_cast<GraphicsProgramInstance>(_gci.program->instance());

					_inst = std::make_shared<PipelineInstance>(gci);
				}
				else if (_binding == VK_PIPELINE_BIND_POINT_COMPUTE)
				{
					PipelineInstance::ComputeCreateInfo cci{
						.app = application(),
						.name = name(),
						.program = std::dynamic_pointer_cast<ComputeProgramInstance>(_cci.program->instance()),
					};

					_inst = std::make_shared<PipelineInstance>(cci);
				}
				else if (_binding == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR)
				{
					
				}

				return AsynchTask::ReturnType{
					.success = true,
				};
			},
			.dependencies = dependecies,
		});

		application()->threadPool().pushTask(_create_instance_task);
	}

	void Pipeline::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	Pipeline::Pipeline(GraphicsCreateInfo const& gci) :
		ParentType(gci.app, gci.name),
		_gci(gci),
		_binding(VK_PIPELINE_BIND_POINT_GRAPHICS),
		_render_pass(gci.render_pass),
		_program(gci.program)
	{
		Callback cb{
			.callback = [&]() {
				destroyInstance();
			},
			.id = this,
		};
		_program->addInvalidationCallback(cb);
		_gci.render_pass->addInvalidationCallback(cb);
		
	}

	Pipeline::Pipeline(ComputeCreateInfo const& cci) :
		ParentType(cci.app, cci.name),
		_cci(cci),
		_binding(VK_PIPELINE_BIND_POINT_COMPUTE),
		_program(cci.program)
	{
		_program->addInvalidationCallback({
			.callback = [&]() {
				destroyInstance();
			},
			.id = this,
		});
	}

	Pipeline::~Pipeline()
	{
		waitForInstanceCreationIFN();
		destroyInstance();
		_program->removeInvalidationCallbacks(this);
	}

	bool Pipeline::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= _program->updateResources(ctx);

		if (!_inst)
		{
			createInstance();
			res = true;
		}

		return res;
	}

	void Pipeline::waitForInstanceCreationIFN()
	{
		if (_create_instance_task)
		{
			_create_instance_task->waitIFN();
			assert(_create_instance_task->isSuccess());
			_create_instance_task = nullptr;
		}
	}

	std::shared_ptr<PipelineInstance> Pipeline::getInstanceWaitIFN()
	{
		waitForInstanceCreationIFN();
		return instance();
	}
}