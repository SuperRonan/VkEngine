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
			std::cout << "Initializing command " << cmd->name() << "\n";
			cmd->init();
		}
	}

	void LinearExecutor::init()
	{
		_blit_to_present = std::make_shared<BlitImage>(BlitImage::CI{
			.app = _app,
			.name = name() + std::string(".BlitToPresent"),
		});
		declare(_blit_to_present);

		preprocessCommands(); 
	}

	void LinearExecutor::beginFrame()
	{
		stackInBetween();
		_in_between = InBetween{
			.fence = std::make_shared<Fence>(_app),
			.semaphore = std::make_shared<Semaphore>(_app),
			.prev_cb = nullptr,
		};
		_aquired = _window->aquireNextImage(_in_between.semaphore, _in_between.fence);
	}

	void LinearExecutor::preparePresentation(std::shared_ptr<ImageView> img_to_present)
	{
		assert(!!_command_buffer_to_submit);
		std::shared_ptr<ImageView> blit_target = _window->view(_aquired.swap_index);

		_blit_to_present->setImages(img_to_present, blit_target);
		_blit_to_present->setRegions();

		execute(_blit_to_present);

		const ResourceState current_state = _context.getImageState(*blit_target);
		const ResourceState desired_state = {
			._access = VK_ACCESS_MEMORY_READ_BIT,
			._layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			._stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Not sure about this one
		};

		if (stateTransitionRequiresSynchronization(current_state, desired_state, true))
		{
			VkImageMemoryBarrier barrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = current_state._access,
				.dstAccessMask = desired_state._access,
				.oldLayout = current_state._layout,
				.newLayout = desired_state._layout,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = *blit_target->image(),
				.subresourceRange = blit_target->range(),
			};

			vkCmdPipelineBarrier(*_command_buffer_to_submit,
				current_state._stage, desired_state._stage, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			const ResourceState final_state = {
				._access = VK_ACCESS_MEMORY_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				._stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // Not sure about this one
			};
			_context.setImageState(*blit_target, final_state);
		}
	}

	void LinearExecutor::present()
	{
		VkSemaphore sem_to_wait = *_in_between.semaphore;
		_window->present(1, &sem_to_wait);
	}

	void LinearExecutor::execute(std::shared_ptr<Command> cmd)
	{
		cmd->execute(_context);
	}

	void LinearExecutor::beginCommandBuffer()
	{
		std::shared_ptr<CommandBuffer>& cb = _command_buffer_to_submit;
		assert(!cb);
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

	void LinearExecutor::stackInBetween()
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

		stackInBetween();

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

		_command_buffer_to_submit = nullptr;
	}

	void LinearExecutor::waitForCurrentCompletion(uint64_t timeout)
	{
		if (!!_in_between.fence)
		{
			_in_between.fence->wait(timeout);
		}
	}
}