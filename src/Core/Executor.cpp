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
		std::shared_ptr<CommandBuffer>& cb = _command_buffer_to_submit;
		cb = std::make_shared<CommandBuffer>(_app->pools().graphics);
		cb->begin();
		_context.setCommandBuffer(cb);
	}

	void Executor::endCommandBufferAndSubmit()
	{
		std::shared_ptr<CommandBuffer> cb = _command_buffer_to_submit;
		_context.setCommandBuffer(nullptr);

		cb->end();

		submit();
	}

	void Executor::prepareSubmission()
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

	void Executor::submit()
	{
		prepareSubmission();

		std::vector<VkCommandBuffer> cbs = { *_command_buffer_to_submit };
		std::vector<VkPipelineStageFlags> wait_stages(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, cbs.size());
		
		VkSubmitInfo submission{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = (uint32_t)cbs.size(),
			.pCommandBuffers = cbs.data(),
		};

		_in_between = InBetween{
			.fence = std::make_shared<Fence>(_app),
			.semaphore = std::make_shared<Semaphore>(_app),
			.prev_cb = _command_buffer_to_submit,
		};

		vkQueueSubmit(_app->queues().graphics, 1, &submission, *_in_between.fence);
	}

	void Executor::waitForCurrentCompletion(uint64_t timeout)
	{
		if (!!_in_between.fence)
		{
			_in_between.fence->wait(timeout);
		}
	}
}