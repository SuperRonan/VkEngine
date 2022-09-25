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
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
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
			.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};
		RenderPass render_pass = RenderPass(application(), at_desc, { at_ref }, { subpass }, { dependency });
		_render_pass = std::make_shared <RenderPass>(std::move(render_pass));

		_framebuffer = std::make_shared<Framebuffer>(_attachements, _render_pass);
	}

	void GraphicsCommand::recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context)
	{
		recordInputSynchronization(cmd, context);
		recordBindings(cmd, context);

		VkExtent2D render_area = {
			.width = _framebuffer->extent().width,
			.height = _framebuffer->extent().height,
		};

		std::vector<VkClearValue> clear_values(_framebuffer->size());
		for (size_t i = 0; i < clear_values.size(); ++i)
		{
			clear_values[i] = VkClearValue{
				.color = VkClearColorValue{.int32{0, 0, 0, 0}},
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
	}

	VertexCommand::VertexCommand(CreateInfo const& ci) :
		GraphicsCommand(ci.app, ci.name, ci.bindings, ci.color_attachements)
	{
		std::shared_ptr<Shader> vert = nullptr, geom = nullptr, frag = nullptr;
		if (!ci.vertex_shader_path.empty())
		{
			vert = std::make_shared<Shader>(ci.app, ci.vertex_shader_path, VK_SHADER_STAGE_VERTEX_BIT, ci.definitions);
		}
		if (!ci.geometry_shader_path.empty())
		{
			geom = std::make_shared<Shader>(ci.app, ci.geometry_shader_path, VK_SHADER_STAGE_GEOMETRY_BIT, ci.definitions);
		}
		if (!ci.fragment_shader_path.empty())
		{
			frag = std::make_shared<Shader>(ci.app, ci.fragment_shader_path, VK_SHADER_STAGE_FRAGMENT_BIT, ci.definitions);
		}
		std::shared_ptr<GraphicsProgram> prog = std::make_shared<GraphicsProgram>(GraphicsProgram::CreateInfo{
			._vertex = vert,
			._geometry = geom, 
			._fragment = frag,
		});

		createGraphicsResources();

		Pipeline::GraphicsCreateInfo gci;
		gci.name = name() + ".Pipeline";
		gci._vertex_input = Pipeline::VertexInputWithoutVertices();
		gci._input_assembly = Pipeline::InputAssemblyPointDefault();
		gci._rasterization = Pipeline::RasterizationDefault();
		gci._multisampling = Pipeline::MultisampleOneSample();
		gci._program = prog;
		gci._render_pass = *_render_pass;
		
		VkViewport viewport = Pipeline::Viewport(extract(_framebuffer->extent()));
		VkRect2D scissor = Pipeline::Scissor(extract(_framebuffer->extent()));


		gci.setViewport({ viewport }, { scissor });
		gci.setColorBlending({ Pipeline::BlendAttachementNoBlending() });
		gci.assemble();

		_pipeline = std::make_shared<Pipeline>(gci);
	}

	void VertexCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	{
		for (auto& mesh : _meshes)
		{
			mesh->recordBind(cmd);
			
			vkCmdDrawIndexed(cmd, (uint32_t)mesh->indicesSize(), 1, 0, 0, 0);
		}
	}

	void FragCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	{
		vkCmdDrawIndexed(cmd, 3, 1, 0, 0, 0);
	}
}