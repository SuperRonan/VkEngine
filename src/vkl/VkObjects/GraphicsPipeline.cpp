
#include <vkl/VkObjects/GraphicsPipeline.hpp>

#include <vkl/VkObjects/DetailedVkFormat.hpp>

namespace vkl
{
	constexpr bool operator==(VkPipelineColorBlendAttachmentState const lhs, VkPipelineColorBlendAttachmentState const& rhs) noexcept
	{
		bool res = lhs.blendEnable == rhs.blendEnable;
		if (lhs.blendEnable && rhs.blendEnable)
		{
			res &= (lhs.srcColorBlendFactor == rhs.srcColorBlendFactor);
			res &= (lhs.dstColorBlendFactor == rhs.dstColorBlendFactor);
			res &= (lhs.colorBlendOp == rhs.colorBlendOp);
			res &= (lhs.srcAlphaBlendFactor == rhs.srcAlphaBlendFactor);
			res &= (lhs.dstAlphaBlendFactor == rhs.dstAlphaBlendFactor);
			res &= (lhs.alphaBlendOp == rhs.alphaBlendOp);
		}
		res &= (lhs.colorWriteMask == rhs.colorWriteMask);
		return res;
	}

	constexpr bool operator!=(VkPipelineColorBlendAttachmentState const lhs, VkPipelineColorBlendAttachmentState const& rhs) noexcept
	{
		bool res = lhs.blendEnable != rhs.blendEnable;
		if (lhs.blendEnable && rhs.blendEnable)
		{
			res |= (lhs.srcColorBlendFactor != rhs.srcColorBlendFactor);
			res |= (lhs.dstColorBlendFactor != rhs.dstColorBlendFactor);
			res |= (lhs.colorBlendOp != rhs.colorBlendOp);
			res |= (lhs.srcAlphaBlendFactor != rhs.srcAlphaBlendFactor);
			res |= (lhs.dstAlphaBlendFactor != rhs.dstAlphaBlendFactor);
			res |= (lhs.alphaBlendOp != rhs.alphaBlendOp);
		}
		res |= (lhs.colorWriteMask != rhs.colorWriteMask);
		return res;
	}

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
		_attachments_blends(ci.attachments_blends),
		_dynamic(ci.dynamic),
		_render_pass(ci.render_pass),
		_subpass_index(ci.subpass_index)
	{
		create();
	}

	void GraphicsPipelineInstance::create()
	{
		VkSubpassDescription2 const& subpass = _render_pass->getSubpasses()[_subpass_index];

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

		assert(_attachments_blends.size32() <= subpass.colorAttachmentCount);
		MyVector<VkPipelineColorBlendAttachmentState> attachments_blend(_attachments_blends.size());
		const bool extra_filter_mask = true;
		for (size_t i = 0; i < _attachments_blends.size(); ++i)
		{
			attachments_blend[i] = _attachments_blends[i].operator VkPipelineColorBlendAttachmentState();	
			if (extra_filter_mask)
			{
				const VkFormat format = _render_pass->getAttachmentDescriptors2()[subpass.pColorAttachments[i].attachment].format;
				const DetailedVkFormat df = DetailedVkFormat::Find(format);
				const VkColorComponentFlags extra_mask = df.getColorComponents();

				attachments_blend[i].colorWriteMask &= extra_mask;
			}
		}
		VkPipelineBlendingState blending_ci;
		_common_blending.extract(blending_ci);
		blending_ci.link(attachments_blend.data(), _attachments_blends.size32());

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
			.pColorBlendState = &blending_ci.ci,
			.pDynamicState = &dynamic_state_ci,
			.layout = layout()->handle(),
			.renderPass = *_render_pass,
			.subpass = _subpass_index,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0,
		};
		VK_CHECK(vkCreateGraphicsPipelines(_app->device(), nullptr, 1, &vk_ci, nullptr, &_handle), "Failed to create a graphics pipeline.");
	}


	VkPipelineMultisampleStateCreateInfo GraphicsPipeline::MultisamplingState::link()
	{
		VkPipelineMultisampleStateCreateInfo res{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = rasterization_samples.valueOr(VK_SAMPLE_COUNT_1_BIT),
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 0,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
		};
		if (min_sample_shading.hasValue())
		{
			const float v = min_sample_shading.value();
			if (v >= 0.0f)
			{
				res.sampleShadingEnable = VK_TRUE;
				res.minSampleShading = std::min(v, 1.0f);
			}
		}
		if (flags.hasValue())
		{
			const Flags v = flags.value();
			if (!!(v & Flags::AlphaToCoverageEnable))
			{
				res.alphaToCoverageEnable = VK_TRUE;
			}
			if (!!(v & Flags::AlphaToOneEnable))
			{
				res.alphaToOneEnable = VK_TRUE;
			}
		}
		return res;
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
		_attachments_blends(ci.attachments_blends),
		_common_blending(ci.common_blending),
		_dynamic(ci.dynamic),
		_render_pass(ci.render_pass),
		_subpass_index(ci.subpass_index)
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
		std::shared_ptr<RenderPassInstance> rpi = _render_pass->instance();
		GraphicsPipelineInstance::CI gci{
			.app = application(),
			.name = name(),
			.vertex_input = _vertex_input,
			.input_assembly = _input_assembly,
			.viewports = _viewports,
			.scissors = _scissors,
			.rasterization = _rasterization.value(),
			.line_raster = {},
			.multisampling = _multisampling.link(),
			.depth_stencil = _depth_stencil,
			.common_blending = _common_blending.valueOr(PipelineBlending{}),
			.dynamic = _dynamic,
			.render_pass = rpi,
			.subpass_index = _subpass_index,
			.program = std::static_pointer_cast<GraphicsProgramInstance>(_program->instance()),
		};
		if (_line_raster.has_value())
		{
			gci.line_raster = _line_raster.value().value();
		}
		gci.attachments_blends.resize(_attachments_blends.size());
		const VkSubpassDescription2 & subpass = rpi->getSubpasses()[_subpass_index];
		for (size_t i = 0; i < _attachments_blends.size(); ++i)
		{
			if (_attachments_blends[i].hasValue())
			{
				gci.attachments_blends[i] = _attachments_blends[i].value();
			}
			else
			{
				AttachmentBlending & blend = gci.attachments_blends[i];

				if (i < subpass.colorAttachmentCount)
				{
					VkAttachmentReference2 const& ref = subpass.pColorAttachments[i];
					// TODO consider the aspect mask for multiplanar formats
					VkAttachmentDescription2 const& desc = rpi->getAttachmentDescriptors2()[ref.attachment];
					DetailedVkFormat df = DetailedVkFormat::Find(desc.format);
					blend = df.getColorComponents();
				}
			}
		}
		_inst = std::make_shared<GraphicsPipelineInstance>(std::move(gci));
	}

	bool GraphicsPipeline::checkInstanceParamsReturnInvalid()
	{
		bool res = false;
		assert(_inst);
		GraphicsPipelineInstance& inst = *static_cast<GraphicsPipelineInstance*>(_inst.get());
		
		do {
			const VkPipelineRasterizationStateCreateInfo & ir = inst._rasterization;
			
			if (_rasterization.polygonMode.hasValue() && ir.polygonMode != _rasterization.polygonMode.value())
			{
				res = true;
				break;
			}

			if (_rasterization.cullMode.hasValue() && ir.cullMode != _rasterization.cullMode.value())
			{
				res = true;
				break;
			}

			if (_rasterization.frontFace.hasValue() && ir.frontFace != _rasterization.frontFace.value())
			{
				res = true;
				break;
			}


			if (_multisampling.rasterization_samples.hasValue() && inst._multisampling.rasterizationSamples != _multisampling.rasterization_samples.value())
			{
				res = true;
				break;
			}

			if (_multisampling.min_sample_shading.hasValue())
			{
				const float v = _multisampling.min_sample_shading.value();
				if (v >= 0.0f)
				{
					if (inst._multisampling.sampleShadingEnable == VK_FALSE || inst._multisampling.minSampleShading != v)
					{
						res = true;
						break;
					}
				}
				else if(inst._multisampling.sampleShadingEnable != VK_FALSE)
				{
					res = true;
					break;
				}
			}
			
			if (_multisampling.flags.hasValue())
			{
				const MultisamplingState::Flags flags = _multisampling.flags.value();
				MultisamplingState::Flags inst_flags = MultisamplingState::Flags::None;
				if (inst._multisampling.alphaToCoverageEnable)
				{
					inst_flags |= MultisamplingState::Flags::AlphaToCoverageEnable;
				}
				if (inst._multisampling.alphaToOneEnable)
				{
					inst_flags |= MultisamplingState::Flags::AlphaToOneEnable;
				}
				if (flags != inst_flags)
				{
					res = true;
					break;
				}
			}

			bool use_any_blend_constant = false;
			if (_attachments_blends.size() == inst._attachments_blends.size())
			{
				for (size_t i = 0; i < _attachments_blends.size(); ++i)
				{
					if (_attachments_blends[i].hasValue())
					{
						const AttachmentBlending& value = _attachments_blends[i].value();
						if (!AttachmentBlending::Equivalent(value, inst._attachments_blends[i]))
						{
							res = true;
							break;
						}
						else if (value.usesConstantBlendFactor())
						{
							use_any_blend_constant |= true;
						}
					}
				}
				if (res)
				{
					break;
				}
			}
			else
			{
				res = true;
				break;
			}

			if (_common_blending.hasValue())
			{
				const PipelineBlending v = _common_blending.value();
				if (!PipelineBlending::Equivalent(v, inst._common_blending, use_any_blend_constant))
				{
					res = true;
					break;
				}
			}

			if (_line_raster.has_value())
			{
				assert(inst._line_raster.has_value());
				const VkPipelineRasterizationLineStateCreateInfoEXT& ilr = inst._line_raster.value();
				const LineRasterizationState & lrs = _line_raster.value();
				
				if (lrs.lineRasterizationMode.hasValue() && ilr.lineRasterizationMode != lrs.lineRasterizationMode.value())
				{
					res = true;
					break;
				}

				if (lrs.lineStippleFactor.hasValue() && ilr.lineStippleFactor != lrs.lineStippleFactor.value())
				{
					res = true;
					break;
				}

				if (lrs.lineStipplePattern.hasValue() && ilr.lineStipplePattern != lrs.lineStipplePattern.value())
				{
					res = true;
					break;
				}
			}

		} while(false);

		return res;
	}
}