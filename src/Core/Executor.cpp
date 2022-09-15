#include "Executor.hpp"

namespace vkl
{
	void LinearExecutor::declare(std::shared_ptr<Command> cmd)
	{
		_commands.push_back(cmd);
	}

	void LinearExecutor::preprocessCommands()
	{
		for (std::shared_ptr<Command>& cmd : _commands)
		{
			std::cout << "Initializing " << cmd->name() << "\n";
			cmd->init();
		}
	}

	void LinearExecutor::init()
	{
		preprocessCommands(); 
	}

	void LinearExecutor::execute(std::shared_ptr<Command> cmd)
	{
		cmd->execute(_context);
	}

	void LinearExecutor::beginCommandBuffer()
	{
		std::shared_ptr<CommandBuffer>& cb = _command_buffer_to_submit;
		cb = std::make_shared<CommandBuffer>(_app->pools().graphics);
		cb->begin();
		_context.setCommandBuffer(cb);
	}

	void LinearExecutor::endCommandBufferAndSubmit()
	{
		std::shared_ptr<CommandBuffer> cb = _command_buffer_to_submit;
		_context.setCommandBuffer(nullptr);

		cb->end();

		submit();
	}

	void LinearExecutor::prepareSubmission()
	{
		// Removed finished in betweens
		while(!_previous_in_betweens.empty())
		{
			InBetween& inb = _previous_in_betweens.back();
			assert(!!inb.fence);
			const VkResult res = vkGetFenceStatus(inb.fence->device(), *inb.fence);
			if (res == VK_SUCCESS)
			{
				// We can remove the inb
				_previous_in_betweens.pop();
			}
			else if(res == VK_NOT_READY)
			{
				break;
			}
			else // == VK_ERROR_DEVICE_LOST
			{
				assert(false);
			}
		}
		// Stack the in between
		if (!!_in_between.fence)
		{
			_previous_in_betweens.push(_in_between);
			_in_between = InBetween{};
		}
	}

	void LinearExecutor::submit()
	{
		VkSemaphore sem_to_wait = *_in_between.semaphore;
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		prepareSubmission();

		VkCommandBuffer cb = *_command_buffer_to_submit;

		_in_between = InBetween{
			.fence = std::make_shared<Fence>(_app),
			.semaphore = std::make_shared<Semaphore>(_app),
			.prev_cb = _command_buffer_to_submit,
		};

		VkSemaphore sem_to_signal = *_in_between.semaphore;
		
		VkSubmitInfo submission{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &sem_to_wait,
			.pWaitDstStageMask = &wait_stage,
			.commandBufferCount = 1,
			.pCommandBuffers = &cb,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &sem_to_signal,
		};

		vkQueueSubmit(_app->queues().graphics, 1, &submission, *_in_between.fence);
	}

	void LinearExecutor::waitForCurrentCompletion(uint64_t timeout)
	{
		if (!!_in_between.fence)
		{
			_in_between.fence->wait(timeout);
		}
	}
}