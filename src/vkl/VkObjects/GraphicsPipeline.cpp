
#include <vkl/VkObjects/GraphicsPipeline.hpp>

namespace vkl
{
	GraphicsPipelineInstance::GraphicsPipelineInstance(CreateInfo const& ci) :
		PipelineInstance(PipelineInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.binding = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.program = ci.program,
		}),
		_vertex_input(ci.vertex_input),
		_input_assembly(ci.input_assembly),
		_viewports(ci.viewports),
		_scissors(ci.scissors),
		_rasterization(ci.rasterization),
		_line_raster(ci.line_raster),
		_multisampling(ci.multisampling),
		_depth_stencil(ci.depth_stencil),
		_attachements_blends(ci.attachements_blends),
		_dynamic(ci.dynamic),
		_render_pass(ci.render_pass)
	{
		create();
	}

	void GraphicsPipelineInstance::create()
	{
		VkPipelineVertexInputStateCreateInfo vertex_input_ci;
		if (_vertex_input.has_value())
			vertex_input_ci = _vertex_input->link();

		uint32_t num_viewport = _viewports.size32();
		uint32_t num_scissor = _scissors.size32();
		if (std::find(_dynamic.cbegin(), _dynamic.cend(), VK_DYNAMIC_STATE_VIEWPORT) != _dynamic.cend())
		{
			num_viewport = 1;
		}
		if (std::find(_dynamic.cbegin(), _dynamic.cend(), VK_DYNAMIC_STATE_SCISSOR) != _dynamic.cend())
		{
			num_scissor = 1;
		}

		VkPipelineViewportStateCreateInfo viewport_ci{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = num_viewport,
			.pViewports = _viewports.data(),
			.scissorCount = num_scissor,
			.pScissors = _scissors.data(),
		};

		VkPipelineColorBlendStateCreateInfo blending_ci{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE, // TODO
			.attachmentCount = _attachements_blends.size32(),
			.pAttachments = _attachements_blends.data(),
			.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
		};

		MyVector<VkPipelineShaderStageCreateInfo> shaders_ci(_program->shaders().size());
		for (size_t i = 0; i < shaders_ci.size(); ++i)
		{
			shaders_ci[i] = _program->shaders()[i]->getPipelineShaderStageCreateInfo();
		}

		VkPipelineDynamicStateCreateInfo dynamic_state_ci{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = _dynamic.size32(),
			.pDynamicStates = _dynamic.data(),
		};

		const bool can_line_raster = application()->deviceExtensions().contains(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME);
		if (_line_raster.has_value() && can_line_raster)
		{
			// TODO check raster option against available features
			_rasterization.pNext = &_line_raster.value();
		}
		else
		{
			_rasterization.pNext = nullptr;
		}



		VkGraphicsPipelineCreateInfo vk_ci{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = shaders_ci.size32(),
			.pStages = shaders_ci.data(),
			.pVertexInputState = _vertex_input.has_value() ? &vertex_input_ci : nullptr,
			.pInputAssemblyState = _input_assembly.has_value() ? &_input_assembly.value() : nullptr,
			.pViewportState = &viewport_ci,
			.pRasterizationState = &_rasterization,
			.pMultisampleState = &_multisampling,
			.pDepthStencilState = _depth_stencil.has_value() ? &_depth_stencil.value() : nullptr,
			.pColorBlendState = &blending_ci,
			.pDynamicState = &dynamic_state_ci,
			.layout = layout()->handle(),
			.renderPass = *_render_pass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0,
		};
		VK_CHECK(vkCreateGraphicsPipelines(_app->device(), nullptr, 1, &vk_ci, nullptr, &_handle), "Failed to create a graphics pipeline.");
	}



	GraphicsPipeline::GraphicsPipeline(CreateInfo const& ci):
		Pipeline(Pipeline::CI{
			.app = ci.app,
			.name = ci.name,
			.binding = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.program = ci.program,
			.hold_instance = ci.hold_instance,
		}),
		_vertex_input(ci.vertex_input),
		_input_assembly(ci.input_assembly),
		_viewports(ci.viewports),
		_scissors(ci.scissors),
		_rasterization(ci.rasterization),
		_line_raster(ci.line_raster),
		_multisampling(ci.multisampling),
		_depth_stencil(ci.depth_stencil),
		_attachements_blends(ci.attachements_blends),
		_dynamic(ci.dynamic),
		_render_pass(ci.render_pass)
	{
		if (_render_pass)
		{
			Callback cb{
				.callback = [this]()
				{
					destroyInstanceIFN();
				},
				.id = this,
			};
			_render_pass->setInvalidationCallback(std::move(cb));
		}
	}

	GraphicsPipeline::~GraphicsPipeline()
	{
		if (_render_pass)
		{
			_render_pass->removeInvalidationCallback(this);
		}
	}

	void GraphicsPipeline::createInstanceIFP()
	{
		GraphicsPipelineInstance::CI gci{
			.app = application(),
			.name = name(),
			.vertex_input = _vertex_input,
			.input_assembly = _input_assembly,
			.viewports = _viewports,
			.scissors = _scissors,
			.rasterization = _rasterization,
			.line_raster = _line_raster,
			.multisampling = _multisampling,
			.depth_stencil = _depth_stencil,
			.attachements_blends = _attachements_blends,
			.dynamic = _dynamic,
			.render_pass = _render_pass->instance(),
			.program = std::static_pointer_cast<GraphicsProgramInstance>(_program->instance()),
		};
		_inst = std::make_shared<GraphicsPipelineInstance>(std::move(gci));
	}

	bool GraphicsPipeline::checkInstanceParamsReturnInvalid()
	{
		return false;
	}
}