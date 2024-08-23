#include <vkl/Execution/RenderPassBeginInfo.hpp>

#include <vkl/VkObjects/RenderPass.hpp>
#include <vkl/VkObjects/Framebuffer.hpp>
#include <vkl/VkObjects/CommandBuffer.hpp>
#include <vkl/Execution/ExecutionContext.hpp>

namespace vkl
{
	RenderPassInstance* RenderPassBeginInfo::getRenderPassInstance() const
	{
		RenderPassInstance* res = nullptr;
		if (render_pass)
		{
			res = render_pass.get();
		}
		else if (framebuffer)
		{
			res = framebuffer->renderPass().get();
		}
		return res;
	}

	void RenderPassBeginInfo::exportResources(ResourceUsageList& resources, bool export_for_all_subpasses)
	{
		assert(render_pass || framebuffer);
		FramebufferInstance * fbi = framebuffer.get();
		RenderPassInstance* rpi = getRenderPassInstance();
		
		if (fbi)
		{
			for (size_t i = 0; i < fbi->attachments().size(); ++i)
			{
				if (rpi)
				{
					VkAttachmentDescription2 const& desc = rpi->getAttachmentDescriptors2().data()[i];
					auto const& usage = rpi->getAttachmentUsages()[i];
					resources += ImageViewUsage{
						.ivi = fbi->attachments()[i],
						.begin_state = ResourceState2{
							.access = usage.initial_access,
							.stage = usage.initial_stage,
							.layout = desc.initialLayout,
						},
						.end_state = ResourceState2{
							.access = usage.final_access,
							.stage = usage.final_stage,
							.layout = desc.finalLayout,
						},
						.usage = usage.usage,
					};
				}
				else
				{
					NOT_YET_IMPLEMENTED;
				}
			}

		}
		else if (framebuffer)
		{
			// In the case of dynamic rendering, it is not necessary to create a FramebufferInstance, but we can still get the attachments from the Framebuffer
			NOT_YET_IMPLEMENTED;
		}
	}

	void RenderPassBeginInfo::recordBegin(ExecutionContext& ctx, VkSubpassContents contents)
	{
		_subpass_index = 0;
		if (framebuffer)
		{
			ctx.keepAlive(framebuffer);
		}
		if (render_pass)
		{
			ctx.keepAlive(render_pass);
		}
		RenderPassInstance* rpi = getRenderPassInstance();
		FramebufferInstance* fbi = framebuffer.get();
		if (render_area.extent == makeUniformExtent2D(uint32_t(-1)))
		{
			render_area.extent = extract(fbi->extent());
			if (render_area.offset.x != 0 || render_area.offset.y != 0)
			{
				NOT_YET_IMPLEMENTED;
			}
		}
		if (!ptr_clear_values)
		{
			clear_value_count = clear_values.size32();
			ptr_clear_values = clear_values.data();
		}
		if (rpi->handle())
		{
			VkRenderPassBeginInfo info{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.pNext = nullptr,
				.renderPass = *rpi,
				.framebuffer = *fbi,
				.renderArea = render_area,
				.clearValueCount = clear_value_count,
				.pClearValues = ptr_clear_values,
			};
			VkSubpassBeginInfo subpass{
				.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
				.pNext = nullptr,
				.contents = contents,
			};
			vkCmdBeginRenderPass2(ctx.getCommandBuffer()->handle(), &info, &subpass);
		}
		else
		{
			// Dynamic Rendering
			NOT_YET_IMPLEMENTED;
			VkRenderingInfo info{
				.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
				.pNext = nullptr,
			};
			vkCmdBeginRendering(ctx.getCommandBuffer()->handle(), &info);
		}

		ctx.renderingInfo() = ExecutionContext::RenderingInfo{
			.render_pass = render_pass ? render_pass : framebuffer->renderPass(),
			.framebuffer = framebuffer,
			.area = render_area,
			.subpass_index = 0,
		};
	}

	void RenderPassBeginInfo::recordNextSubpass(ExecutionContext& ctx, VkSubpassContents contents)
	{
		RenderPassInstance* rpi = getRenderPassInstance();
		if (rpi->handle())
		{
			VkSubpassBeginInfo begin{
				.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
				.pNext = nullptr,
				.contents = contents,
			};
			VkSubpassEndInfo end{
				.sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
				.pNext = nullptr,
			};
			vkCmdNextSubpass2(ctx.getCommandBuffer()->handle(), &begin, &end);
		}
		else
		{
			// Dynamic Rendering
			NOT_YET_IMPLEMENTED;
		}
		++_subpass_index;
		ctx.renderingInfo().subpass_index = _subpass_index;
	}

	void RenderPassBeginInfo::exportSubpassResources(uint32_t index, ResourceUsageList& resources)
	{
		RenderPassInstance* rpi = getRenderPassInstance();
		FramebufferInstance* fbi = framebuffer.get();
		for (size_t i = 0; i < fbi->attachments().size(); ++i)
		{
			if (rpi)
			{
				VkAttachmentDescription2 const& desc = rpi->getAttachmentDescriptors2().data()[i];
				auto const& usage = rpi->getAttachmentUsages()[i];
				if (usage.first_subpass == index)
				{
					resources += ImageViewUsage{
						.ivi = fbi->attachments()[i],
						.begin_state = ResourceState2{
							.access = usage.initial_access,
							.stage = usage.initial_stage,
							.layout = desc.initialLayout,
						},
						.end_state = ResourceState2{
							.access = usage.final_access,
							.stage = usage.final_stage,
							.layout = desc.finalLayout,
						},
						.usage = usage.usage,
					};
				}
			}
			else
			{
				NOT_YET_IMPLEMENTED;
			}
		}
	}

	void RenderPassBeginInfo::recordEnd(ExecutionContext & ctx)
	{
		RenderPassInstance* rpi = getRenderPassInstance();
		if (rpi->handle())
		{
			VkSubpassEndInfo end{
				.sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
				.pNext = nullptr,
			};
			vkCmdEndRenderPass2(ctx.getCommandBuffer()->handle(), &end);
		}
		else
		{
			vkCmdEndRendering(ctx.getCommandBuffer()->handle());
		}

		ctx.renderingInfo().clear();
	}
}