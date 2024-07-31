#include <vkl/Execution/RenderPassBeginInfo.hpp>

#include <vkl/VkObjects/RenderPass.hpp>
#include <vkl/VkObjects/Framebuffer.hpp>
#include <vkl/VkObjects/CommandBuffer.hpp>

namespace vkl
{
	void RenderPassBeginInfo::exportResources(ResourceUsageList& resources)
	{
		assert(render_pass || framebuffer);
		RenderPassInstance * rpi = nullptr;
		FramebufferInstance * fbi = nullptr;
		if (framebuffer)
		{
			fbi = framebuffer->instance().get();
		}
		if (render_pass)
		{
			rpi = render_pass->instance().get();
		}
		else
		{
			assert(fbi);
			rpi = fbi->renderPass().get();
		}
		
		if (fbi)
		{
			for (size_t i = 0; i < fbi->colorSize(); ++i)
			{
				
			}
		}
	}

	void RenderPassBeginInfo::recordBegin(CommandBuffer& cmd)
	{

	}

	void RenderPassBeginInfo::recordNextSubpass(CommandBuffer& cmd, uint32_t index)
	{

	}
}