#include "GraphicsCommand.hpp"

namespace vkl
{
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
		std::shared_ptr<CommandBuffer> cmd = context.getCurrentCommandBuffer();
		recordCommandBuffer(*cmd, context);
	}

	void DrawMeshCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	{
		for (auto& mesh : _meshes)
		{
			mesh->recordBind(cmd);
			vkCmdDrawIndexed(cmd, (uint32_t)mesh->indicesSize(), 1, 0, 0, 0);
		}
	}

	void FragCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	{
		vkCmdDrawIndexed(cmd, 4, 1, 0, 0, 0);
	}
}