#include "GraphicsCommand.hpp"

#include <thatlib/src/stl_ext/const_forward.hpp>
#include <thatlib/src/core/Concepts.hpp>

namespace vkl
{

	GraphicsCommandNode::GraphicsCommandNode(CreateInfo const& ci):
		ShaderCommandNode(ShaderCommandNode::CI{
			.app = ci.app,
			.name = ci.name,
		})
	{}

	void GraphicsCommandNode::clear()
	{
		ShaderCommandNode::clear();

		_render_pass.reset();
		_framebuffer.reset();
		_depth_stencil.reset();

		_clear_values.clear();
		_clear_color.reset();
		_clear_depth_stencil.reset();
		
		_viewport = {};
	}

	void GraphicsCommandNode::execute(ExecutionContext& ctx)
	{
		CommandBuffer & cmd = *ctx.getCommandBuffer();

		const VkExtent2D render_area = extract(_framebuffer->extent());

		const size_t num_clear_values = (_clear_color.has_value() || _clear_depth_stencil.has_value()) ? (_framebuffer->colorSize() + 1) : 0;

		_clear_values.resize(num_clear_values);
		if (num_clear_values)
		{
			if (_clear_color.has_value())
			{
				const VkClearValue cv = { .color = _clear_color.value() };
				std::fill_n(_clear_values.begin(), _framebuffer->colorSize(), cv);
			}
			if (_clear_depth_stencil.has_value())
			{
				_clear_values.back() = VkClearValue{ .depthStencil = _clear_depth_stencil.value() };
			}
		}

		const std::string pass_name = name();

		VkRenderPassBeginInfo begin = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = *_render_pass,
			.framebuffer = *_framebuffer,
			.renderArea = VkRect2D{.offset = makeUniformOffset2D(0), .extent = render_area},
			.clearValueCount = static_cast<uint32_t>(num_clear_values),
			.pClearValues = num_clear_values ? _clear_values.data() : nullptr,
		};

		vkCmdBeginRenderPass(cmd, &begin, VK_SUBPASS_CONTENTS_INLINE);
		{
			recordBindings(cmd, ctx);
			if (false)
			{

			}
			else
			{
				VkViewport viewport = GraphicsPipeline::Viewport(extract(_framebuffer->extent()));
				VkRect2D scissor = GraphicsPipeline::Scissor(extract(_framebuffer->extent()));
				vkCmdSetViewport(cmd, 0, 1, &viewport);
				vkCmdSetScissor(cmd, 0, 1, &scissor);
			}
			recordDrawCalls(ctx);
		}
		vkCmdEndRenderPass(cmd);

		ctx.keepAlive(_pipeline);
		ctx.keepAlive(_framebuffer);
		ctx.keepAlive(_render_pass);
	}


	GraphicsCommand::GraphicsCommand(CreateInfo const& ci) :
		ShaderCommand(ShaderCommand::CreateInfo{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts,
		}),
		_topology(ci.topology),
		_polygon_mode(ci.polygon_mode),
		_cull_mode(ci.cull_mode),
		_vertex_input_desc(ci.vertex_input_description),
		_attachements(ci.targets),
		_depth_stencil(ci.depth_stencil),
		_write_depth(ci.write_depth),
		_depth_compare_op(ci.depth_compare_op),
		_stencil_front_op(ci.stencil_front_op),
		_stencil_back_op(ci.stencil_back_op),
		_clear_color(ci.clear_color),
		_clear_depth_stencil(ci.clear_depth_stencil),
		_blending(ci.blending),
		_line_raster_mode(ci.line_raster_mode),
		_draw_type(ci.draw_type)
	{
		if (ci.extern_framebuffer.has_value())
		{
			_framebuffer = ci.extern_framebuffer.value();
		}
		else
		{
			_framebuffer = std::shared_ptr<Framebuffer>(nullptr);
		}
	}

	void GraphicsCommand::createGraphicsResources()
	{
		using namespace std::containers_append_operators;
		const bool use_extern_fb = _framebuffer.index() == 1;
		const uint32_t n_color = use_extern_fb ? std::get<1>(_framebuffer).color_attachments.size32() : static_cast<uint32_t>(_attachements.size());
		
		MyVector<RenderPass::AttachmentDescription2> at_desc(n_color);
		MyVector<VkAttachmentReference2> at_ref(n_color);

		MyVector<VkSubpassDependency2> dependencies;


		for (size_t i = 0; i < n_color; ++i)
		{
			const std::optional<VkClearColorValue> & clear_color = _clear_color;
			const std::optional<VkPipelineColorBlendAttachmentState> & opt_blending = _blending;

			const VkImageLayout layout = application()->options().getLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
			VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

			if (clear_color.has_value())	load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
			if (opt_blending.has_value())
			{
				const VkPipelineColorBlendAttachmentState & blending = opt_blending.value();
				if (blending.blendEnable)
				{
					load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
					// We could go in details of the blend factors
				}
			}
			
			Dyn<VkFormat> attachment_format;
			Dyn<VkSampleCountFlagBits> attachment_samples;
			if (use_extern_fb)
			{
				attachment_format = std::get<1>(_framebuffer).color_attachments[i].format;
				attachment_samples = std::get<1>(_framebuffer).color_attachments[i].samples;
			}
			else
			{
				attachment_format = _attachements[i]->format();
				attachment_samples = _attachements[i]->image()->sampleCount();
			}

			at_desc[i] = RenderPass::AttachmentDescription2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.pNext = nullptr,
				.flags = 0,
				.format = attachment_format,
				.samples = attachment_samples,
				.loadOp = load_op,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = layout,
				.finalLayout = layout,
			};
			at_ref[i] = VkAttachmentReference2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
				.pNext = nullptr,
				.attachment = uint32_t(i),
				.layout = layout,
			};
		}
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

		const bool depth_attachment = [&]() {
			bool res = false;
			if (use_extern_fb)
			{
				res = std::get<1>(_framebuffer).detph_stencil_attchement.has_value();
			}
			else
			{
				res = _depth_stencil.operator bool();
			}
			return res;
		}();

		if (depth_attachment)
		{
			const bool write_depth = _write_depth.value_or(false);

			Dyn<VkFormat> depth_stencil_format;
			Dyn<VkSampleCountFlagBits> depth_stencil_samples;
			if (use_extern_fb)
			{
				ExternFramebufferInfo const& fb = std::get<1>(_framebuffer);
				assert(fb.detph_stencil_attchement.has_value());
				depth_stencil_format = fb.detph_stencil_attchement->format;
				depth_stencil_samples = fb.detph_stencil_attchement->samples;
			}
			else
			{
				assert(_depth_stencil.operator bool());
				depth_stencil_format = _depth_stencil->format();
				depth_stencil_samples = _depth_stencil->image()->sampleCount();
			}

			VkImageAspectFlags aspect = getImageAspectFromFormat(*depth_stencil_format);
			const bool depth = (aspect & VK_IMAGE_ASPECT_DEPTH_BIT) && _depth_compare_op.has_value();
			const bool stencil = (aspect & VK_IMAGE_ASPECT_STENCIL_BIT) && (_stencil_front_op.has_value() || _stencil_back_op.has_value());
			assert(depth || stencil);
			VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			if (depth && !stencil)
			{
				layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			}
			else if (!depth && stencil)
			{
				layout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
			}
			layout = application()->options().getLayout(layout, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

			VkAttachmentLoadOp depth_load_op = VK_ATTACHMENT_LOAD_OP_LOAD, stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			VkAttachmentStoreOp depth_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE, stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			if (depth)
			{
				if (_clear_depth_stencil.has_value())
				{
					depth_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
				}

				if (write_depth)
				{
					depth_store_op = VK_ATTACHMENT_STORE_OP_STORE;
				}
			}

			if (stencil)
			{
				if (_stencil_back_op.has_value() || _stencil_front_op.has_value())
				{
					// TODO go in the details of the stencil op
					stencil_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
					stencil_store_op = VK_ATTACHMENT_STORE_OP_STORE;
				}
			
				if (_clear_depth_stencil.has_value())
				{
					stencil_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
				}
			}


			at_desc.push_back(RenderPass::AttachmentDescription2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.pNext = nullptr,
				.flags = 0,
				.format = depth_stencil_format,
				.samples = depth_stencil_samples,
				.loadOp = depth_load_op,
				.storeOp = depth_store_op,
				.stencilLoadOp = stencil_load_op,
				.stencilStoreOp = stencil_store_op,
				.initialLayout = layout,
				.finalLayout = layout,
			});
			at_ref.push_back(VkAttachmentReference2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
				.pNext = nullptr,
				.attachment = static_cast<uint32_t>(at_desc.size() - 1),
				.layout = layout,
			});

			VkSubpassDependency2 depth_stencil_dependency = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
				.pNext = nullptr,
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				.srcAccessMask = VK_ACCESS_NONE,
				.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, // TODO if read depth
			};
			dependencies.push_back(depth_stencil_dependency);
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
			.pDepthStencilAttachment = depth_attachment ? (at_ref.data() + n_color) : nullptr, // Warning this is unsafe, the ptr is copied, assuming the data is copied later
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr,
		};

		bool multiview = false;
		if (use_extern_fb)
		{
			multiview = std::get<1>(_framebuffer).multiview;
		}
		_render_pass = std::make_shared <RenderPass>(RenderPass::CI{
			.app = application(),
			.name = name() + ".RenderPass",
			.attachement_descriptors = at_desc,
			.attachement_ref_per_subpass = {at_ref},
			.subpasses = {subpass},
			.dependencies = dependencies,
			.last_is_depth_stencil = depth_attachment,
			.multiview = multiview,
		});

		if (!use_extern_fb)
		{
			_framebuffer = std::make_shared<Framebuffer>(Framebuffer::CI{
				.app = application(),
				.name = name() + ".Framebuffer",
				.render_pass = _render_pass,
				.targets = _attachements,
				.depth_stencil = _depth_stencil,
			});
		}
	}

	void GraphicsCommand::populateFramebufferResources(GraphicsCommandNode & node)
	{
		node._render_pass = _render_pass->instance();
		if (_framebuffer.index() == 0)
		{
			node._framebuffer = std::get<0>(_framebuffer)->instance();
			node._depth_stencil = _depth_stencil ? _depth_stencil->instance() : nullptr;
		}

		if (node._framebuffer->depthStencil() && !node._depth_stencil)
		{
			node._depth_stencil = node._framebuffer->depthStencil();
		}

		node._clear_color = _clear_color;
		node._clear_depth_stencil = _clear_depth_stencil;

		for (size_t i = 0; i < node._framebuffer->colorSize(); ++i)
		{
			const VkAttachmentDescription2 & at_desc = node._render_pass->getAttachementDescriptors2()[i];
			VkAccessFlags2 access = VK_ACCESS_2_NONE;
			if (at_desc.storeOp == VK_ATTACHMENT_STORE_OP_STORE || at_desc.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
			{
				access |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			}
			if (at_desc.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
			{
				access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			}
			node.resources() += ImageViewUsage{
				.ivi = node._framebuffer->textures()[i],
				.begin_state = ResourceState2{
					.access = access, 
					.layout = at_desc.initialLayout,
					.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				},
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			};
			// TODO end state IFN
		}
		if (node._depth_stencil)
		{
			const VkAttachmentDescription2& at_desc = node._render_pass->getAttachementDescriptors2().back();
			
			VkAccessFlags2 access = VK_ACCESS_2_NONE;
			VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

			if (at_desc.storeOp == VK_ATTACHMENT_STORE_OP_STORE || at_desc.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE || at_desc.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR || at_desc.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
			{
				access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			}
			if (at_desc.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD || at_desc.stencilStoreOp == VK_ATTACHMENT_LOAD_OP_LOAD)
			{
				access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			}

			node.resources() += ImageViewUsage{
				.ivi = node._depth_stencil,
				.begin_state = ResourceState2{
					.access = access,
					.layout = at_desc.initialLayout,
					.stage = stage,
				},
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			};
			// TODO end state IFN
		}
	}

	void GraphicsCommand::createPipeline()
	{
		GraphicsPipeline::CI gci;
		gci.app = application();
		gci.name = name() + ".Pipeline";
		gci.vertex_input = _vertex_input_desc;
		gci.input_assembly = GraphicsPipeline::InputAssemblyDefault(_topology);
		gci.rasterization = GraphicsPipeline::RasterizationDefault(_cull_mode, _polygon_mode, 1.0f);
		
		if (_line_raster_mode.has_value())
		{
			gci.line_raster = GraphicsPipeline::LineRasterization(_line_raster_mode.value());
		}

		// TODO from attachements
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

		// should be dynamic?
		gci.multisampling = GraphicsPipeline::MultisampleState(samples);
		gci.program = _program;
		gci.render_pass = _render_pass;

		const bool use_extern_fb = _framebuffer.index() == 1;
		const bool depth_attachment = [&]() {
			bool res = false;
			if (use_extern_fb)
			{
				res = std::get<1>(_framebuffer).detph_stencil_attchement.has_value();
			}
			else
			{
				res = _depth_stencil.operator bool();
			}
			return res;
		}();
		if (depth_attachment)
		{
			const VkImageAspectFlags aspect = [&](){
				VkImageAspectFlags res = 0;
				if (use_extern_fb)
				{
					res = getImageAspectFromFormat(std::get<1>(_framebuffer).detph_stencil_attchement->format.value());
				}
				else
				{
					res = _depth_stencil->range().value().aspectMask;
				}
				return res;
			}();
			const bool depth = (aspect & VK_IMAGE_ASPECT_DEPTH_BIT) && _depth_compare_op.has_value();
			const bool stencil = (aspect & VK_IMAGE_ASPECT_STENCIL_BIT) && (_stencil_front_op.has_value() || _stencil_back_op.has_value());
			gci.depth_stencil = VkPipelineDepthStencilStateCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.depthTestEnable = depth ? VK_TRUE : VK_FALSE,
				.depthWriteEnable = _write_depth.value_or(false) ? VK_TRUE : VK_FALSE,
				.depthCompareOp = _depth_compare_op.value_or(VK_COMPARE_OP_LESS),
				.depthBoundsTestEnable = VK_FALSE,
				.stencilTestEnable = stencil ? VK_TRUE : VK_FALSE,
				.minDepthBounds = 0.0,
				.maxDepthBounds = 1.0,
			};
			if (stencil)
			{
				const VkStencilOpState default_op{
					.failOp = VK_STENCIL_OP_KEEP,
					.passOp = VK_STENCIL_OP_KEEP,
					.depthFailOp = VK_STENCIL_OP_KEEP,
					.compareOp = VK_COMPARE_OP_ALWAYS,
					.compareMask = 0x00,
					.writeMask = 0x00,
					.reference = 0x00,
				};
				gci.depth_stencil.value().front = _stencil_front_op.value_or(default_op);
				gci.depth_stencil.value().back = _stencil_back_op.value_or(default_op);
			}
		}

		if (false)
		{
			//VkViewport viewport = Pipeline::Viewport(extract(*_framebuffer->extent()));
			//VkRect2D scissor = Pipeline::Scissor(extract(*_framebuffer->extent()));

			//gci.viewports = { viewport };
			//gci.scissors = { scissor };
		}
		else
		{
			gci.dynamic = { VK_DYNAMIC_STATE_VIEWPORT , VK_DYNAMIC_STATE_SCISSOR };
		}
		const size_t n_color = [&]() {
			size_t res = 0;
			if (_framebuffer.index() == 0)
			{
				res = std::get<0>(_framebuffer)->colorSize();
			}
			else if (_framebuffer.index() == 1)
			{
				res = std::get<1>(_framebuffer).color_attachments.size();
			}
			return res;
		}();
		MyVector<VkPipelineColorBlendAttachmentState> blending(n_color, _blending.value_or(GraphicsPipeline::BlendAttachementNoBlending()));
		gci.attachements_blends = blending;


		_pipeline = std::make_shared<GraphicsPipeline>(std::move(gci));
	}

	void GraphicsCommand::init()
	{
		ShaderCommand::init();
	}

	bool GraphicsCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= _render_pass->updateResources(ctx);

		res |= ShaderCommand::updateResources(ctx);

		if (_framebuffer.index() == 0)
		{
			std::shared_ptr<Framebuffer> fb = std::get<0>(_framebuffer);
			if (fb)
			{
				fb->updateResources(ctx);
			}
		}

		return res;
	}











	VertexCommandNode::VertexCommandNode(CreateInfo const& ci):
		GraphicsCommandNode(GraphicsCommandNode::CI{
			.app = ci.app,
			.name = ci.name,
		})
	{}

	void VertexCommandNode::clear()
	{
		GraphicsCommandNode::clear();

		_vertex_buffers.clear();
		_draw_list.clear();

		_vb_bind.clear();
		_vb_offsets.clear();
		_draw_type = DrawType::MAX_ENUM;
	}

	void VertexCommandNode::recordDrawCalls(ExecutionContext& ctx)
	{
		CommandBuffer & cmd = *ctx.getCommandBuffer();
		const uint32_t set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
		const std::shared_ptr<PipelineLayoutInstance>& layout = _pipeline->program()->pipelineLayout();
		
		for (DrawCallInfo& to_draw : _draw_list)
		{
			if (to_draw.name_size != 0)
			{
				ctx.pushDebugLabel(_strings.get(Range{.begin = to_draw.name_begin, .len = static_cast<Index>(to_draw.name_size), }), true);
			}
			std::shared_ptr<DescriptorSetAndPoolInstance> set = to_draw.set;
			if (set)
			{
				if (set->exists() && !set->empty())
				{
					ctx.graphicsBoundSets().bindOneAndRecord(set_index, set, layout);
					ctx.keepAlive(set);
				}
			}
			recordPushConstantIFN(cmd, to_draw.pc_begin, to_draw.pc_size, to_draw.pc_offset);

			if (to_draw.index_buffer.buffer)
			{
				const BufferAndRangeInstance & bari = to_draw.index_buffer;
				vkCmdBindIndexBuffer(cmd, bari.buffer->handle(), bari.range.begin, to_draw.index_type);
			}
			if (to_draw.num_vertex_buffers > 0)
			{
				_vb_bind.resize(to_draw.num_vertex_buffers);
				_vb_offsets.resize(to_draw.num_vertex_buffers);
				for (size_t i = 0; i < _vb_bind.size(); ++i)
				{
					const BufferAndRangeInstance & bari = _vertex_buffers.data()[to_draw.vertex_buffer_begin + i];
					_vb_bind[i] = bari.buffer->handle();
					_vb_offsets[i] = bari.range.begin;
				}
				vkCmdBindVertexBuffers(cmd, 0, to_draw.num_vertex_buffers, _vb_bind.data(), _vb_offsets.data());
			}

			switch (_draw_type)
			{
			case DrawType::Draw:
				vkCmdDraw(cmd, to_draw.draw_count, to_draw.instance_count, 0, 0);
				break;
			case DrawType::DrawIndexed:
				vkCmdDrawIndexed(cmd, to_draw.draw_count, to_draw.instance_count, 0, 0, 0);
				break;
			case DrawType::IndirectDraw:
				vkCmdDrawIndirect(cmd, to_draw.indirect_draw_buffer.buffer->handle(), to_draw.indirect_draw_buffer.range.begin, to_draw.draw_count, to_draw.indirect_draw_stride);
				break;
			case DrawType::IndirectDrawIndexed:
				vkCmdDrawIndexedIndirect(cmd, to_draw.indirect_draw_buffer.buffer->handle(), to_draw.indirect_draw_buffer.range.begin, to_draw.draw_count, to_draw.indirect_draw_stride);
				break;
			default:
				assertm(false, "Unsupported draw call type");
				break;
			}
			if (to_draw.name_size != 0)
			{
				ctx.popDebugLabel();
			}
		}

		if (ctx.framePerfCounters())
		{
			ctx.framePerfCounters()->draw_calls += _draw_list.size();
		}
	}




	VertexCommand::VertexCommand(CreateInfo const& ci) :
		GraphicsCommand(GraphicsCommand::CreateInfo{
			.app = ci.app,
			.name = ci.name,
			.topology = ci.topology,
			.polygon_mode = ci.polygon_mode,
			.cull_mode = ci.cull_mode,
			.vertex_input_description = ci.vertex_input_desc,
			.line_raster_mode = ci.line_raster_mode,
			.sets_layouts = ci.sets_layouts,
			.bindings = ci.bindings,
			.extern_framebuffer = ci.extern_framebuffer,
			.targets = ci.color_attachements,
			.depth_stencil = ci.depth_stencil,
			.write_depth = ci.write_depth,
			.depth_compare_op = ci.depth_compare_op,
			.stencil_front_op = ci.stencil_front_op,
			.stencil_back_op = ci.stencil_back_op,
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
		Dyn<DefinitionsList> defs = [this](DefinitionsList & res)
		{
			res = _shaders.definitions.getCachedValue();
		};
		if (!_shaders.vertex_path.empty())
		{
			vert = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".vert",
				.source_path = _shaders.vertex_path,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.definitions = defs,
			});
		}
		if (!_shaders.tess_control_path.empty())
		{
			tess_control = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".tess_control",
				.source_path = _shaders.tess_control_path,
				.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
				.definitions = defs,
			});
		}
		if (!_shaders.tess_eval_path.empty())
		{
			tess_eval = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".tess_eval",
				.source_path = _shaders.tess_eval_path,
				.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
				.definitions = defs,
			});
		}
		if (!_shaders.geometry_path.empty())
		{
			geom = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".geom",
				.source_path = _shaders.geometry_path,
				.stage = VK_SHADER_STAGE_GEOMETRY_BIT,
				.definitions = defs,
			});
		}
		if (!_shaders.fragment_path.empty())
		{
			frag = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".frag",
				.source_path = _shaders.fragment_path,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.definitions = defs,
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

	struct VertexCommandTemplateProcessor
	{
		template <class DCIRef>
		static void DrawInfo_pushBack(VertexCommand::DrawInfo& that, DCIRef&& dci)
		{
			that.calls.push_back(VertexCommand::MyDrawCallInfo{
				.name_size = static_cast<uint32_t>(dci.name.size()),
				.pc_size = dci.pc_size,
				.pc_offset = dci.pc_offset,
				.draw_count = dci.draw_count,
				.instance_count = dci.instance_count,
				.indirect_draw_stride = dci.indirect_draw_stride,
				.indirect_draw_buffer = dci.indirect_draw_buffer,
				.index_buffer = dci.index_buffer,
				.index_type = dci.index_type,
				.num_vertex_buffers = dci.num_vertex_buffers,
				.set = std::forward<std::shared_ptr<DescriptorSetAndPool>>(dci.set),
			});
			VertexCommand::MyDrawCallInfo& mdci = that.calls.back();
			if (!dci.name.empty())
			{
				mdci.name_begin = that._strings.pushBack(dci.name, true);
			}
			if (dci.pc_data && dci.pc_size != 0)
			{
				mdci.pc_begin = that._data.pushBack(dci.pc_data, dci.pc_size);
			}
			if (dci.vertex_buffers && dci.num_vertex_buffers != 0)
			{
				using VB = typename std::remove_pointer<decltype(dci.vertex_buffers)>::type;
				constexpr const bool can_move = !std::is_const<typename std::remove_reference<DCIRef>::type>::value && !std::is_const<VB>::value;
				if constexpr (can_move)
				{
					mdci.vertex_buffer_begin = that._vertex_buffers.pushBackMove(dci.vertex_buffers, dci.num_vertex_buffers);
				}
				else
				{
					mdci.vertex_buffer_begin = that._vertex_buffers.pushBack(dci.vertex_buffers, dci.num_vertex_buffers);
				}
			}
		}
		

		template <that::concepts::UniversalReference<VertexCommand::DrawInfo> DrawInfoRef>
		static void populateDrawCallsResources(VertexCommand& that, VertexCommandNode& node, DrawInfoRef&& di)
		{
			const uint32_t invocation_set_index = that.application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
			std::shared_ptr<DescriptorSetLayoutInstance> layout = that._pipeline->program()->instance()->reflectionSetsLayouts()[invocation_set_index];
			node._draw_type = di.draw_type;
			

			
			node._vertex_buffers.resize(di._vertex_buffers.size());
			for (size_t i = 0; i < node._vertex_buffers.size(); ++i)
			{
				node._vertex_buffers.data()[i] = di._vertex_buffers.data()[i].getInstance();
			}

			node._draw_list.resize(di.calls.size());
			for (size_t i = 0; i < node._draw_list.size(); ++i)
			{
				auto& to_draw = di.calls[i];
				VertexCommandNode::DrawCallInfo& node_to_draw = node._draw_list[i];
				
				node_to_draw.name_begin = to_draw.name_begin;
				node_to_draw.pc_begin = to_draw.pc_begin;
				
				node_to_draw.name_size = to_draw.name_size;
				node_to_draw.pc_size = to_draw.pc_size;

				node_to_draw.pc_offset = to_draw.pc_offset;
				node_to_draw.draw_count = to_draw.draw_count;
				 
				node_to_draw.instance_count = to_draw.instance_count;
				node_to_draw.indirect_draw_stride = to_draw.indirect_draw_stride;
				
				node_to_draw.indirect_draw_buffer = to_draw.indirect_draw_buffer.getInstance();

				node_to_draw.index_buffer = to_draw.index_buffer.getInstance();
				
				node_to_draw.index_type = to_draw.index_type;
				node_to_draw.num_vertex_buffers = to_draw.num_vertex_buffers;

				node_to_draw.vertex_buffer_begin = to_draw.vertex_buffer_begin;

				if (layout)
				{
					assert(!!to_draw.set);
					node_to_draw.set = to_draw.set->instance();
					that.populateDescriptorSet(node, *to_draw.set->instance(), *layout);
				}

				if (node_to_draw.index_buffer.buffer)
				{
					node.resources() += BufferUsage{
						.bari = node_to_draw.index_buffer,
						.begin_state = ResourceState2{
							.access = VK_ACCESS_2_INDEX_READ_BIT,
							.stage = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
						},
						.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
					};
				}
				if (node_to_draw.indirect_draw_buffer.buffer)
				{
					node.resources() += BufferUsage{
						.bari = node_to_draw.indirect_draw_buffer,
						.begin_state = ResourceState2{
							.access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
							.stage = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
						},
						.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
					};
				}
				for (uint32_t i = 0; i < node_to_draw.num_vertex_buffers; ++i)
				{
					node.resources() += BufferUsage{
						.bari = node._vertex_buffers.data()[node_to_draw.vertex_buffer_begin + i],
						.begin_state = ResourceState2{
							.access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
							.stage = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
						},
						.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					};
				}
			}
		}

		template <that::concepts::UniversalReference<VertexCommand::DrawInfo> DrawInfoRef>
		static std::shared_ptr<ExecutionNode> getExecutionNode(VertexCommand& that, RecordContext& ctx, DrawInfoRef&& di)
		{
			std::shared_ptr<VertexCommandNode> node = that._exec_node_cache.getCleanNode<VertexCommandNode>([&]() {
				return std::make_shared<VertexCommandNode>(VertexCommandNode::CI{
					.app = that.application(),
				});
			});

			node->setName(that.name());

			const uint32_t shader_set_index = that.application()->descriptorBindingGlobalOptions().shader_set;
			const uint32_t invocation_set_index = that.application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
			if (di.extern_framebuffer)
			{
				node->_framebuffer = di.extern_framebuffer->instance();
			}
			that.populateBoundResources(*node, ctx.graphicsBoundSets(), shader_set_index + 1);
			that.populateFramebufferResources(*node);
			populateDrawCallsResources<DrawInfoRef>(that, *node, std::forward<VertexCommand::DrawInfo>(di));

			node->_data = std::forward<that::ExDS>(di._data);
			node->_strings = std::forward<that::ExSS>(di._strings);

			node->pc_begin = di.pc_begin;
			node->pc_size = di.pc_size;
			node->pc_offset = di.pc_offset;

			return node;
		}

		template <that::concepts::UniversalReference<VertexCommand::SingleDrawInfo> SDIRef>
		static Executable with_SDI(VertexCommand& that, SDIRef&& sdi)
		{
			static thread_local VertexCommand::DrawInfo di;
			di.clear();

			di.draw_type = DrawType::Draw;
			di.viewport = sdi.viewport;
			di.setPushConstant(sdi.pc_data, sdi.pc_size, sdi.pc_offset);
			di.pushBack(VertexCommand::DrawCallInfo{
				.draw_count = sdi.draw_count,
				.instance_count = sdi.instance_count,
			});

			return that.with(std::move(di));
		}
	};

	void VertexCommand::DrawInfo::clear()
	{
		ShaderCommandList::clear();
		draw_type = DrawType::MAX_ENUM;

		viewport = {};
		calls.clear();
		_vertex_buffers.clear();
		extern_framebuffer.reset();
	}
	
	void VertexCommand::DrawInfo::pushBack(DrawCallInfoConst const& dci)
	{
		VertexCommandTemplateProcessor::DrawInfo_pushBack<DrawCallInfoConst const&>(*this, dci);
	}

	void VertexCommand::DrawInfo::pushBack(DrawCallInfoConst&& dci)
	{
		VertexCommandTemplateProcessor::DrawInfo_pushBack<DrawCallInfoConst&&>(*this, std::move(dci));
	}

	void VertexCommand::DrawInfo::pushBack(DrawCallInfo const& dci)
	{
		VertexCommandTemplateProcessor::DrawInfo_pushBack<DrawCallInfo const&>(*this, dci);
	}

	void VertexCommand::DrawInfo::pushBack(DrawCallInfo&& dci)
	{
		VertexCommandTemplateProcessor::DrawInfo_pushBack<DrawCallInfo&&>(*this, std::move(dci));
	}


	std::shared_ptr<ExecutionNode> VertexCommand::getExecutionNode(RecordContext& ctx, DrawInfo const& di)
	{
		return VertexCommandTemplateProcessor::getExecutionNode<DrawInfo const&>(*this, ctx, di);
	}

	std::shared_ptr<ExecutionNode> VertexCommand::getExecutionNode(RecordContext& ctx, DrawInfo && di)
	{
		auto res = VertexCommandTemplateProcessor::getExecutionNode<DrawInfo &&>(*this, ctx, std::move(di));
		di.clear();
		return res;
	}

	std::shared_ptr<ExecutionNode> VertexCommand::getExecutionNode(RecordContext& ctx)
	{
		SingleDrawInfo sdi{
			.draw_count = _draw_count.value(),
			.instance_count = 1,
			.pc_data = _pc.data(),
			.pc_size = _pc.size32(),
			.pc_offset = 0,
		};
		return with(std::move(sdi))(ctx);
	}

	Executable VertexCommand::with(DrawInfo const& di)
	{
		return [this, di](RecordContext& ctx)
		{
			return getExecutionNode(ctx, di);
		};
	}

	Executable VertexCommand::with(DrawInfo && di)
	{
		return [this, _di = std::move(di)](RecordContext& ctx) mutable
		{
			return getExecutionNode(ctx, std::move(_di));
		};
	}

	Executable VertexCommand::with(SingleDrawInfo const& sdi)
	{
		return VertexCommandTemplateProcessor::with_SDI<SingleDrawInfo const&>(*this, sdi);
	}

	Executable VertexCommand::with(SingleDrawInfo && sdi)
	{
		return VertexCommandTemplateProcessor::with_SDI<SingleDrawInfo&&>(*this, std::move(sdi));
	}

	bool VertexCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		_shaders.definitions.value();

		res |= GraphicsCommand::updateResources(ctx);

		return res;
	}





	MeshCommandNode::MeshCommandNode(CreateInfo const& ci) :
		GraphicsCommandNode(GraphicsCommandNode::CI{
			.app = ci.app,
			.name = ci.name,
		})
	{}

	void MeshCommandNode::clear()
	{
		GraphicsCommandNode::clear();

		_draw_list.clear();
		_draw_type = DrawType::MAX_ENUM;
	}

	void MeshCommandNode::recordDrawCalls(ExecutionContext & ctx) 
	{
		const auto& _vkCmdDrawMeshTasksEXT = application()->extFunctions()._vkCmdDrawMeshTasksEXT;
		CommandBuffer & cmd = *ctx.getCommandBuffer();

		const uint32_t set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
		const std::shared_ptr<PipelineLayoutInstance>& layout = _pipeline->program()->pipelineLayout();

		for (auto& to_draw : _draw_list)
		{
			if (to_draw.name_size != 0)
			{
				ctx.pushDebugLabel(_strings.get(Range{.begin = to_draw.name_begin, .len = to_draw.name_size}), true);
			}
			const VkExtent3D & work = to_draw.extent;

			const std::shared_ptr<DescriptorSetAndPoolInstance> & set = to_draw.set;
			if (set && (set->exists() && !set->empty()))
			{
				ctx.graphicsBoundSets().bindOneAndRecord(set_index, set, layout);
				ctx.keepAlive(set);
			}
			recordPushConstantIFN(cmd, to_draw.pc_begin, to_draw.pc_size, to_draw.pc_offset);

			switch (_draw_type)
			{
			case DrawType::Dispatch:
				_vkCmdDrawMeshTasksEXT(cmd, work.width, work.height, work.depth);
				break;
			default:
				assertm(false, "Unsupported draw call type");
				break;
			}
			if (to_draw.name_size != 0)
			{
				ctx.popDebugLabel();
			}
		}

		if (ctx.framePerfCounters())
		{
			ctx.framePerfCounters()->draw_calls += _draw_list.size();
		}
	}


	MeshCommand::MeshCommand(CreateInfo const& ci) :
		GraphicsCommand(GraphicsCommand::CreateInfo{
			.app = ci.app,
			.name = ci.name,
			.polygon_mode = ci.polygon_mode,
			.cull_mode = ci.cull_mode,
			.line_raster_mode = ci.line_raster_mode,
			.sets_layouts = ci.sets_layouts,
			.bindings = ci.bindings,
			.extern_framebuffer = ci.extern_framebuffer,
			.targets = ci.color_attachements,
			.depth_stencil = ci.depth_stencil,
			.write_depth = ci.write_depth,
			.depth_compare_op = ci.depth_compare_op,
			.stencil_front_op = ci.stencil_front_op,
			.stencil_back_op = ci.stencil_back_op,
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
		Dyn<DefinitionsList> defs = [this](DefinitionsList& res)
		{
			res = _shaders.definitions.getCachedValue();
		};
		if (!_shaders.task_path.empty())
		{
			task = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".task",
				.source_path = _shaders.task_path,
				.stage = VK_SHADER_STAGE_TASK_BIT_EXT,
				.definitions = defs,
			});
		}
		if (!_shaders.mesh_path.empty())
		{
			mesh = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".mesh",
				.source_path = _shaders.mesh_path,
				.stage = VK_SHADER_STAGE_MESH_BIT_EXT,
				.definitions = defs,
			});
		}
		if (!_shaders.fragment_path.empty())
		{
			frag = std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".frag",
				.source_path = _shaders.fragment_path,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.definitions = defs,
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

	struct MeshCommandTemplateProcessor
	{
		template <that::concepts::UniversalReference<MeshCommand::DrawCallInfo> DCIRef>
		static void DrawInfo_pushBack(MeshCommand::DrawInfo& that, DCIRef&& dci)
		{
			that.draw_list.push_back(MeshCommand::MyDrawCallInfo{
				.name_size = static_cast<uint32_t>(dci.name.size()),
				.pc_size = dci.pc_size,
				.pc_offset = dci.pc_offset,
				.extent = dci.extent,
				.set = std::forward<std::shared_ptr<DescriptorSetAndPool>>(dci.set),
			});
			MeshCommand::MyDrawCallInfo & mdci = that.draw_list.back();
			if (mdci.name_size != 0)
			{
				mdci.name_begin = that._strings.pushBack(dci.name, true);
			}
			if (dci.pc_data && mdci.pc_size != 0)
			{
				mdci.pc_begin = that._data.pushBack(dci.pc_data, mdci.pc_size);
			}
		}

		template <that::concepts::UniversalReference<MeshCommand::DrawInfo> DrawInfoRef>
		static void populateDrawCallsResources(MeshCommand& that, MeshCommandNode& node, DrawInfoRef&& di)
		{
			const uint32_t invocation_set_index = that.application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
			std::shared_ptr<DescriptorSetLayoutInstance> layout = that._pipeline->program()->instance()->reflectionSetsLayouts()[invocation_set_index];

			node._draw_type = di.draw_type;
			node._draw_list.resize(di.draw_list.size());

			for (size_t i = 0; i < di.draw_list.size(); ++i)
			{
				auto& to_draw = di.draw_list[i];
				MeshCommandNode::DrawCallInfo& node_to_draw = node._draw_list[i];
				
				node_to_draw.name_begin = to_draw.name_begin;
				node_to_draw.pc_begin = to_draw.pc_begin;
				node_to_draw.name_size = to_draw.name_size;
				node_to_draw.pc_size = to_draw.pc_size;
				node_to_draw.pc_offset = to_draw.pc_offset;
				node_to_draw.extent = di.dispatch_threads ? that.getWorkgroupsDispatchSize(to_draw.extent) : to_draw.extent;

				if (layout)
				{
					assert(!!to_draw.set);
					node_to_draw.set = to_draw.set->instance();
					that.populateDescriptorSet(node, *to_draw.set->instance(), *layout);
				}
			}
		}

		template <that::concepts::UniversalReference<MeshCommand::DrawInfo> DrawInfoRef>
		static std::shared_ptr<ExecutionNode> getExecutionNode(MeshCommand& that, RecordContext& ctx, DrawInfoRef&& di)
		{
			assert(that.application()->availableFeatures().mesh_shader_ext.meshShader);
			std::shared_ptr<MeshCommandNode> node = that._exec_node_cache.getCleanNode<MeshCommandNode>([&]() {
				return std::make_shared<MeshCommandNode>(MeshCommandNode::CI{
					.app = that.application(),
				});
			});

			node->setName(that.name());

			if (di.extern_framebuffer)
			{
				node->_framebuffer = di.extern_framebuffer->instance();
			}

			const uint32_t shader_set_index = that.application()->descriptorBindingGlobalOptions().shader_set;
			const uint32_t invocation_set_index = that.application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
			ctx.graphicsBoundSets().bind(that.application()->descriptorBindingGlobalOptions().shader_set, that._set->instance());

			that.populateBoundResources(*node, ctx.graphicsBoundSets(), shader_set_index + 1);
			that.populateFramebufferResources(*node);
			populateDrawCallsResources<DrawInfoRef>(that, *node,  std::forward<MeshCommand::DrawInfo>(di));

			node->_data = std::forward<that::ExDS>(di._data);
			node->_strings = std::forward<that::ExSS>(di._strings);

			node->pc_begin = di.pc_begin;
			node->pc_size = di.pc_size;
			node->pc_offset = di.pc_offset;

			return node;
		}

		template <that::concepts::UniversalReference<MeshCommand::SingleDrawInfo> SDIRef>
		static Executable with_SDI(MeshCommand& that, SDIRef&& sdi)
		{
			static thread_local MeshCommand::DrawInfo di;
			di.clear();
			di.dispatch_threads = sdi.dispatch_threads.value_or(that._dispatch_threads);
			di.setPushConstant(sdi.pc_data, sdi.pc_size, sdi.pc_offset);
			di.draw_type = DrawType::Dispatch;
			di.pushBack(MeshCommand::DrawCallInfo{
				.set = std::forward<std::shared_ptr<DescriptorSetAndPool>>(sdi.set),
			});
			MeshCommand::MyDrawCallInfo & mdci = di.draw_list.back();
			if (sdi.extent.has_value())
			{
				mdci.extent = sdi.extent.value();
			}
			else
			{
				mdci.extent = that._extent.value();
			}
			return that.with(std::move(di));
		}
	};
	void MeshCommand::DrawInfo::clear()
	{
		ShaderCommandList::clear();
		draw_type = DrawType::MAX_ENUM;
		dispatch_threads = false;
		draw_list.clear();
		extern_framebuffer.reset();
	}

	void MeshCommand::DrawInfo::pushBack(DrawCallInfo const& dci)
	{
		MeshCommandTemplateProcessor::DrawInfo_pushBack<DrawCallInfo const&>(*this, dci);
	}

	void MeshCommand::DrawInfo::pushBack(DrawCallInfo && dci)
	{
		MeshCommandTemplateProcessor::DrawInfo_pushBack<DrawCallInfo &&>(*this, std::move(dci));
	}

	std::shared_ptr<ExecutionNode> MeshCommand::getExecutionNode(RecordContext& ctx, DrawInfo const& di)
	{
		return MeshCommandTemplateProcessor::getExecutionNode<DrawInfo const&>(*this, ctx, di);
	}

	std::shared_ptr<ExecutionNode> MeshCommand::getExecutionNode(RecordContext& ctx, DrawInfo && di)
	{
		auto res = MeshCommandTemplateProcessor::getExecutionNode<DrawInfo &&>(*this, ctx, std::move(di));
		di.clear();
		return res;
	}

	std::shared_ptr<ExecutionNode> MeshCommand::getExecutionNode(RecordContext& ctx)
	{
		SingleDrawInfo sdi{
			.extent = _extent,
			.dispatch_threads = _dispatch_threads,
			.pc_data = _pc.data(),
			.pc_size = _pc.size32(),
			.pc_offset = 0,
		};

		return with(std::move(sdi))(ctx);
	}

	Executable MeshCommand::with(DrawInfo const& di)
	{
		return [this, di](RecordContext& ctx)
		{
			return getExecutionNode(ctx, di);
		};
	}

	Executable MeshCommand::with(DrawInfo && di)
	{
		return [this, _di = std::move(di)](RecordContext& ctx)
		{
			return getExecutionNode(ctx, std::move(_di));
		};
	}

	Executable MeshCommand::with(SingleDrawInfo const& sdi)
	{
		return MeshCommandTemplateProcessor::with_SDI<SingleDrawInfo const&>(*this, sdi);
	}

	Executable MeshCommand::with(SingleDrawInfo && sdi)
	{
		return MeshCommandTemplateProcessor::with_SDI<SingleDrawInfo &&>(*this, std::move(sdi));
	}

	bool MeshCommand::updateResources(UpdateContext& ctx)
	{
		bool res = false;

		_shaders.definitions.value();

		res |= GraphicsCommand::updateResources(ctx);

		return res;
	}


	//void FragCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	//{
	//	vkCmdDrawIndexed(cmd, 3, 1, 0, 0, 0);
	//}
}