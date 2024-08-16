#include <vkl/Commands/GraphicsCommand.hpp>

#include <that/stl_ext/const_forward.hpp>
#include <that/core/Concepts.hpp>

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

		_render_pass_begin_info.clear();
		_depth_stencil.reset();

		_clear_values.clear();
		_clear_color.reset();
		_clear_depth_stencil.reset();
		
		_viewport = {};
	}

	void GraphicsCommandNode::exportFramebufferResources()
	{
		_render_pass_begin_info.exportResources(_resources, true);
	}

	void GraphicsCommandNode::execute(ExecutionContext& ctx)
	{
		CommandBuffer & cmd = *ctx.getCommandBuffer();

		_render_pass_begin_info.recordBegin(ctx);
		{
			recordBindings(cmd, ctx);
			if (false)
			{

			}
			else
			{
				VkExtent2D render_area = extract(_render_pass_begin_info.framebuffer->extent());
				VkViewport viewport = GraphicsPipeline::Viewport(render_area);
				VkRect2D scissor = GraphicsPipeline::Scissor(render_area);
				vkCmdSetViewport(cmd, 0, 1, &viewport);
				vkCmdSetScissor(cmd, 0, 1, &scissor);
			}
			recordDrawCalls(ctx);
		}
		_render_pass_begin_info.recordEnd(ctx);

		ctx.keepAlive(_pipeline);
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
		_color_attachements(ci.color_attachments),
		_depth_stencil(ci.depth_stencil_attachment),
		_fragment_shading_rate_image(ci.fragment_shading_rate_image),
		_fragment_shading_rate_texel_size(ci.fragment_shading_rate_texel_size),
		_inline_multisampling(ci.inline_multisampling),
		_view_mask(ci.view_mask),
		_use_external_renderpass(ci.extern_render_pass),
		_write_depth(ci.write_depth),
		_depth_compare_op(ci.depth_compare_op),
		_stencil_front_op(ci.stencil_front_op),
		_stencil_back_op(ci.stencil_back_op),
		_line_raster_state(ci.line_raster_state),
		_common_shader_definitions(ci.definitions),
		_draw_type(ci.draw_type)
	{
		if (ci.extern_render_pass)
		{
			_render_pass = ci.extern_render_pass;
		}
	}

	void GraphicsCommand::createGraphicsResources()
	{
		using namespace std::containers_append_operators;

		if (!_use_external_renderpass)
		{
			RenderPass::CI render_pass_ci{
				.app = application(),
				.name = name() + ".RenderPass",
				.mode = RenderPass::Mode::Automatic,
			};
			render_pass_ci.subpasses.push_back(SubPassDescription2{
				.view_mask = _view_mask,
				.read_only_depth = _write_depth.value_or(false),
				.read_only_stencil = !(_stencil_front_op.has_value() || _stencil_back_op.has_value()),
				.disallow_merging = application()->options().render_pass_disallow_merging,
				.auto_preserve_all_other_attachments = false, // No need
				.shading_rate_texel_size = _fragment_shading_rate_texel_size,
				.inline_multisampling = _inline_multisampling,
			});
			MyVector<AttachmentDescription2>& attachments = render_pass_ci.attachments;
			SubPassDescription2& subpass = render_pass_ci.subpasses.front();
			Framebuffer::CI framebuffer_ci{
				.app = application(),
				.name = name() + ".Framebuffer",
			};
			uint32_t attachment_count = 0;
			const uint32_t color_base = attachment_count;
			subpass.colors.resize(_color_attachements.size());
			attachment_count += _color_attachements.size32();
			const uint32_t resolve_base = attachment_count;
			const uint32_t resolve_count = std::count_if(_color_attachements.begin(), _color_attachements.end(), [](ColorAttachment const& a) {return a.resolved.operator bool(); });
			if (resolve_count)
			{
				attachment_count += resolve_count;
				subpass.inputs.resize(_color_attachements.size());
			}
			const uint32_t depth_stencil_base = attachment_count;
			if (_depth_stencil.view)
			{
				attachment_count += 1;
			}
			const uint32_t fragment_shading_rate_base = attachment_count;
			if (_fragment_shading_rate_image)
			{
				attachment_count += 1;
			}
			attachments.resize(attachment_count);
			framebuffer_ci.attachments.resize(attachment_count);
			uint32_t resolve_counter = 0;
			for (uint32_t i = 0; i < _color_attachements.size32(); ++i)
			{
				ColorAttachment const& ca = _color_attachements[i];
				AttachmentDescription2& dst = attachments[color_base + i];
				dst.format = ca.view->format();
				dst.samples = ca.view->image()->sampleCount();
				dst.flags = [this, i]()
				{
					AttachmentDescription2::Flags flags = AttachmentDescription2::Flags::None;
					if (_color_attachements[i].clear_value)
					{
						flags |= AttachmentDescription2::Flags::Clear;
					}
					else if(_color_attachements[i].blending && _color_attachements[i].blending.value().blendEnable)
					{
						flags |= AttachmentDescription2::Flags::Blend;
					}
					else
					{
						flags |= AttachmentDescription2::Flags::OverWrite;
					}
					return flags;
				};
				subpass.colors[i].index = color_base + i;
				if (ca.view->range())
				{
					// The Dyn is not forwarded, but is it necessary?
					subpass.colors[i].aspect = ca.view->range().value().aspectMask;
				}
				framebuffer_ci.attachments[color_base + i] = ca.view;
				if (resolve_count > 0)
				{
					AttachmentDescription2& resolve_dst = attachments[resolve_base + i];
					if (ca.resolved)
					{
						attachments[resolve_counter].flags = AttachmentDescription2::Flags::OverWrite;
						attachments[resolve_counter].format = ca.resolved->format();
						attachments[resolve_counter].samples = ca.resolved->image()->sampleCount();
						subpass.resolves[i].index = resolve_counter;
						framebuffer_ci.attachments[resolve_counter] = ca.resolved;
						if (ca.resolved->range())
						{
							subpass.resolves[i].aspect = ca.resolved->range().value().aspectMask;
						}
						++resolve_counter;
					}
				}
			}
			if (_depth_stencil.view)
			{
				AttachmentDescription2& dst = attachments[depth_stencil_base];
				VkImageAspectFlags aspect = getImageAspectFromFormat(_depth_stencil.view->format().value()); 
				if (_depth_stencil.view->range().hasValue())
				{
					aspect &= _depth_stencil.view->range().value().aspectMask;
				}
				dst.format = _depth_stencil.view->format();
				dst.samples = _depth_stencil.view->image()->sampleCount();
				const bool depth = aspect & VK_IMAGE_ASPECT_DEPTH_BIT;
				const bool stencil = aspect & VK_IMAGE_ASPECT_STENCIL_BIT;
				dst.flags = [this, depth, stencil]
				{
					AttachmentDescription2::Flags flags = AttachmentDescription2::Flags::None;
					if (depth)
					{
						const bool read_only = _write_depth.value_or(false);
						if (_depth_stencil.clear_depth)
						{
							flags |= AttachmentDescription2::Flags::LoadOpClear;
						}
						else
						{
							flags |= AttachmentDescription2::Flags::LoadOpLoad;
						}
						if (read_only)
						{
							flags |= AttachmentDescription2::Flags::StoreOpNone;
						}
						else
						{
							flags |= AttachmentDescription2::Flags::StoreOpStore;
						}
					}
					if (stencil)
					{
						const bool read_only = !(_stencil_front_op.has_value() || _stencil_back_op.has_value());
						if (_depth_stencil.clear_stencil)
						{
							flags |= AttachmentDescription2::Flags::StencilLoadOpClear;
						}
						else
						{
							flags |= AttachmentDescription2::Flags::StencilLoadOpLoad;
						}
						if (read_only)
						{
							flags |= AttachmentDescription2::Flags::StencilStoreOpNone;
						}
						else
						{
							flags |= AttachmentDescription2::Flags::StencilStoreOpStore;
						}
					}
					return flags;
				};

				subpass.depth_stencil.index = depth_stencil_base;
				subpass.depth_stencil.aspect = aspect;
				framebuffer_ci.attachments[depth_stencil_base] = _depth_stencil.view;
			}
			if (_fragment_shading_rate_image)
			{
				// The LoadOp and StoreOp is ignored 
				attachments[fragment_shading_rate_base].format = _fragment_shading_rate_image->format();
				attachments[fragment_shading_rate_base].samples = _fragment_shading_rate_image->image()->sampleCount();
				subpass.fragment_shading_rate.index = fragment_shading_rate_base;
				framebuffer_ci.attachments[fragment_shading_rate_base] = _fragment_shading_rate_image;
			}
			
			_render_pass = std::make_shared<RenderPass>(std::move(render_pass_ci));
			framebuffer_ci.render_pass = _render_pass;
			_framebuffer = std::make_shared<Framebuffer>(std::move(framebuffer_ci));
		}
	}

	void GraphicsCommand::populateFramebufferResources(GraphicsCommandNode & node)
	{
		RenderPassBeginInfo& info = node._render_pass_begin_info;
		if (_render_pass && !info.render_pass)
		{
			info.render_pass = _render_pass->instance();
		}
		if (_framebuffer && !info.framebuffer)
		{
			info.framebuffer = _framebuffer->instance();
		}

		node.exportFramebufferResources();
	}

	void GraphicsCommand::createPipeline()
	{
		GraphicsPipeline::CI gci;
		gci.app = application();
		gci.name = name() + ".Pipeline";
		gci.vertex_input = _vertex_input_desc;
		gci.input_assembly = GraphicsPipeline::InputAssemblyDefault(_topology);
		gci.rasterization = GraphicsPipeline::RasterizationState{
			.polygonMode = _polygon_mode,
			.cullMode = _cull_mode,
		};
		
		gci.line_raster = _line_raster_state;

		Dyn<VkSampleCountFlagBits> samples = [this]()
		{
			VkSampleCountFlagBits res = VK_SAMPLE_COUNT_1_BIT;
			for (size_t i = 0; i < _color_attachements.size(); ++i)
			{
				const auto& sc = _color_attachements[i].view->image()->sampleCount();
				if (sc)
				{
					res = std::max(res, sc.value());
				}
			}
			return res;
		};
		// should be dynamic?
		gci.multisampling = GraphicsPipeline::MultisamplingState{
			.rasterization_samples = samples,
		};
		gci.program = _program;
		gci.render_pass = _render_pass;

		const uint32_t depth_stencil_index = _render_pass->subpasses().front().depth_stencil.index;
		if (depth_stencil_index != VK_ATTACHMENT_UNUSED)
		{
			const VkImageAspectFlags aspect = [&](){
				VkImageAspectFlags res = 0;
				if (_use_external_renderpass)
				{
					res = getImageAspectFromFormat(_render_pass->attachments()[depth_stencil_index].format.value()) & _render_pass->subpasses().front().depth_stencil.aspect;
				}
				else
				{
					res = _depth_stencil.view->range().value().aspectMask;
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

		gci.attachements_blends.resize(_color_attachements.size());
		for (uint32_t i = 0; i < _color_attachements.size32(); ++i)
		{
			gci.attachements_blends[i] = _color_attachements[i].blending;
		}


		_pipeline = std::make_shared<GraphicsPipeline>(std::move(gci));
	}

	void GraphicsCommand::init()
	{
		ShaderCommand::init();
	}

	bool GraphicsCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		_common_shader_definitions.value();

		if (!_use_external_renderpass)
		{
			res |= _render_pass->updateResources(ctx);
		}

		res |= ShaderCommand::updateResources(ctx);

		if (_framebuffer)
		{
			res |= _framebuffer->updateResources(ctx);
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
			.line_raster_state = ci.line_raster_state,
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
			.line_raster_state = ci.line_raster_state,
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