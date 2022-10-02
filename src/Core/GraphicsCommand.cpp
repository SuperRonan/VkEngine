#include "GraphicsCommand.hpp"

namespace vkl
{
	GraphicsCommand::GraphicsCommand(
		VkApplication * app, 
		std::string const& name, 
		std::vector<ShaderBindingDescriptor> const& bindings,
		std::vector<std::shared_ptr<ImageView>> const& targets
	):
		ShaderCommand(app, name, bindings),
		_attachements(targets)
	{}

	void GraphicsCommand::createGraphicsResources()
	{
		const size_t n = _attachements.size();
		std::vector<VkAttachmentDescription> at_desc(n);
		std::vector<VkAttachmentReference> at_ref(n);
		for (size_t i = 0; i < n; ++i)
		{
			at_desc[i] = VkAttachmentDescription{
				.flags = 0,
				.format = _attachements[i]->format(),
				.samples = _attachements[i]->image()->sampleCount(),
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_GENERAL,
			};
			at_ref[i] = VkAttachmentReference{
				.attachment = uint32_t(i),
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};
		}
		VkSubpassDescription subpass = {
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = static_cast<uint32_t>(n),
			.pColorAttachments= at_ref.data(),
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = nullptr,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr,
		};
		VkSubpassDependency dependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};
		RenderPass render_pass = RenderPass(application(), at_desc, { at_ref }, { subpass }, { dependency });
		_render_pass = std::make_shared <RenderPass>(std::move(render_pass));

		_framebuffer = std::make_shared<Framebuffer>(_attachements, _render_pass);
	}

	void GraphicsCommand::declareGraphicsResources()
	{
		for (size_t i = 0; i < _framebuffer->size(); ++i)
		{
			std::shared_ptr<ImageView> view = _framebuffer->textures()[i];
			_resources.push_back(Resource{
				._images = {view},
				._begin_state = ResourceState{
					._access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // TODO add read bit if alpha blending 
					._layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					._stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				},
				._end_state = ResourceState{
					._access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // TODO add read bit if alpha blending 
					._layout = VK_IMAGE_LAYOUT_GENERAL,
					._stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				},
				._image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			});
		}
		// TODO depth buffer
	}

	void GraphicsCommand::init()
	{
		createProgramIFN();
		createGraphicsResources();
	}

	void GraphicsCommand::recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context)
	{
		recordInputSynchronization(cmd, context);
		recordBindings(cmd, context);

		VkExtent2D render_area = extract(_framebuffer->extent());

		std::vector<VkClearValue> clear_values(_framebuffer->size());
		for (size_t i = 0; i < clear_values.size(); ++i)
		{
			clear_values[i] = VkClearValue{
				.color = VkClearColorValue{.int32{0, 0, 0, 1}},
			};
		}

		VkRenderPassBeginInfo begin = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = *_render_pass,
			.framebuffer = *_framebuffer,
			.renderArea = VkRect2D{.offset = VkOffset2D{0, 0}, .extent = render_area},
			.clearValueCount = (uint32_t)clear_values.size(),
			.pClearValues = clear_values.data(),
		};

		vkCmdBeginRenderPass(cmd, &begin, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *_pipeline);
			recordBindings(cmd, context);
			recordDraw(cmd, context);
		}
		vkCmdEndRenderPass(cmd);
	}

	void GraphicsCommand::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordCommandBuffer(*cmd, context);
		declareResourcesEndState(context);
	}

	VertexCommand::VertexCommand(CreateInfo const& ci) :
		GraphicsCommand(ci.app, ci.name, ci.bindings, ci.color_attachements),
		_shaders(ShaderPaths{
			.vertex_path = ci.vertex_shader_path, 
			.geometry_path = ci.geometry_shader_path, 
			.fragment_path = ci.fragment_shader_path, 
			.definitions = ci.definitions 
		}),
		_draw_count(ci.draw_count),
		_meshes(ci.meshes)
	{
		
	}

	void VertexCommand::createProgramIFN()
	{
		std::shared_ptr<Shader> vert = nullptr, geom = nullptr, frag = nullptr;
		if (!_shaders.vertex_path.empty())
		{
			vert = std::make_shared<Shader>(application(), _shaders.vertex_path, VK_SHADER_STAGE_VERTEX_BIT, _shaders.definitions);
		}
		if (!_shaders.geometry_path.empty())
		{
			geom = std::make_shared<Shader>(application(), _shaders.geometry_path, VK_SHADER_STAGE_GEOMETRY_BIT, _shaders.definitions);
		}
		if (!_shaders.fragment_path.empty())
		{
			frag = std::make_shared<Shader>(application(), _shaders.fragment_path, VK_SHADER_STAGE_FRAGMENT_BIT, _shaders.definitions);
		}
		_program = std::make_shared<GraphicsProgram>(GraphicsProgram::CreateInfo{
			._vertex = vert,
			._geometry = geom,
			._fragment = frag,
		});
	}

	void VertexCommand::init()
	{
		GraphicsCommand::init();
		createGraphicsResources();

		Pipeline::GraphicsCreateInfo gci;
		gci.name = name() + ".Pipeline";
		gci._vertex_input = Pipeline::VertexInputWithoutVertices();
		gci._input_assembly = Pipeline::InputAssemblyPointDefault();
		gci._rasterization = Pipeline::RasterizationDefault();
		gci._multisampling = Pipeline::MultisampleOneSample();
		gci._program = _program;
		gci._render_pass = *_render_pass;

		VkViewport viewport = Pipeline::Viewport(extract(_framebuffer->extent()));
		VkRect2D scissor = Pipeline::Scissor(extract(_framebuffer->extent()));


		gci.setViewport({ viewport }, { scissor });
		gci.setColorBlending({ Pipeline::BlendAttachementNoBlending() });
		gci.assemble();

		_pipeline = std::make_shared<Pipeline>(gci);

		resolveBindings();
		declareGraphicsResources();
		declareDescriptorSetsResources();
		writeDescriptorSets();
	}

	void VertexCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	{
		if (_draw_count.has_value())
		{
			vkCmdDraw(cmd, _draw_count.value(), 1, 0, 0);
		}
		else
		{
			for (auto& mesh : _meshes)
			{
				mesh->recordBind(cmd);
				vkCmdDrawIndexed(cmd, (uint32_t)mesh->indicesSize(), 1, 0, 0, 0);
			}
		}
	}

	void FragCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	{
		vkCmdDrawIndexed(cmd, 3, 1, 0, 0, 0);
	}
}