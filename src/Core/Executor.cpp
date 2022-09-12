#include "Executor.hpp"

namespace vkl
{
	void Executor::declare(std::shared_ptr<Command> cmd)
	{
		_commands.push_back(cmd);
	}

	void Executor::preprocessCommands()
	{
		for (std::shared_ptr<Command>& cmd : _commands)
		{
			std::cout << "Initializing " << cmd->name() << "\n";
			cmd->init();
		}
	}

	void Executor::init()
	{
		preprocessCommands(); 
	}

	void Executor::execute(std::shared_ptr<Command> cmd)
	{
		cmd->execute(_context);
	}

	void Executor::beginCommandBuffer()
	{
		std::shared_ptr<CommandBuffer> cb = std::make_shared<CommandBuffer>(_app->pools().graphics);
		cb->begin();
		_context.setCommandBuffer(cb);
	}

	void Executor::endCommandBufferAndSubmit()
	{
		std::shared_ptr<CommandBuffer> cb = _context.getCommandBuffer();
		_context.setCommandBuffer(nullptr);

		cb->end();

		_command_buffers_to_submit.push_back(cb);
		submit();
	}

	void Executor::prepareSubmission()
	{
		if (!!_in_between.fence)
		{

		}
	}

	void Executor::submit()
	{
		prepareSubmission();

		std::vector<VkCommandBuffer> cbs(_command_buffers_to_submit.size());
		std::vector<VkPipelineStageFlags> wait_stages(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, cbs.size());
		for (size_t i = 0; i < cbs.size(); ++i)
		{
			cbs[i] = *_command_buffers_to_submit[i];
		}
		VkSubmitInfo submission{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = (uint32_t)cbs.size(),
			.pCommandBuffers = cbs.data(),
		};

		vkQueueSubmit(_app->queues().graphics, 1, &submission, *_in_between.fence);
	}
}