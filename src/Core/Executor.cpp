#include "Executor.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>


namespace vkl
{
	LinearExecutor::LinearExecutor(CreateInfo const& ci) :
		VkObject(ci.app ? ci.app : ci.window->application(), ci.name),
		_window(ci.window),
		_context(&_resources_state, nullptr)
	{
		if (ci.use_ImGui)
		{
			_render_gui = std::make_shared<ImguiCommand>(ImguiCommand::CI{
				.app = application(),
				.name = name() + ".RenderGui",
				.targets = _window->views(),
			});
			declare(_render_gui);
		}
	}

	void LinearExecutor::declare(std::shared_ptr<Command> cmd)
	{
		_commands.push_back(cmd);
	}

	void LinearExecutor::declare(std::shared_ptr<ImageView> view)
	{

	}

	void LinearExecutor::declare(std::shared_ptr<Buffer> buffer)
	{

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

		if (_render_gui)
		{
			beginCommandBuffer();
			{
				ImGui_ImplVulkan_CreateFontsTexture(*_command_buffer_to_submit);
			}
			endCommandBufferAndSubmit();
		}
	}

	void LinearExecutor::beginFrame()
	{
		++_frame_index;
		stackInBetween();
		_in_between = InBetween{
			.prev_cb = nullptr,
			.next_cb = nullptr,
			.fences = {std::make_shared<Fence>(_app, name() + " BeginFrame # " + std::to_string(_frame_index) + " Fence")},
			.semaphore = std::make_shared<Semaphore>(_app, name() + " BeginFrame # " + std::to_string(_frame_index) + " Semaphore"),
		};
		_aquired = _window->aquireNextImage(_in_between.semaphore, _in_between.fences[0]);
		int _ = 0;
	}

	void LinearExecutor::preparePresentation(std::shared_ptr<ImageView> img_to_present, bool render_ImGui)
	{
		assert(!!_command_buffer_to_submit);
		std::shared_ptr<ImageView> blit_target = _window->view(_aquired.swap_index);

		_blit_to_present->setImages(img_to_present, blit_target);
		_blit_to_present->setRegions();

		execute(_blit_to_present);

		if (render_ImGui && _render_gui)
		{
			ImGui::Render();
			execute(_render_gui);
		}

		const ResourceState current_state = _context.getImageState(blit_target);
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
			_context.setImageState(blit_target, final_state);
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

	void LinearExecutor::operator()(std::shared_ptr<Command> cmd)
	{
		return execute(cmd);
	}

	void LinearExecutor::beginCommandBuffer()
	{
		std::shared_ptr<CommandBuffer>& cb = _command_buffer_to_submit;
		assert(!cb);
		cb = std::make_shared<CommandBuffer>(CommandBuffer::CI{
			.name = name() + " Frame # " + std::to_string(_frame_index) + " CommandBuffer",
			.pool = _app->pools().graphics 
		});
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
			InBetween& inb = _previous_in_betweens.front();
			assert(!inb.fences.empty());
			bool all_success = true;
			for (auto& fence : inb.fences)
			{
				const VkResult res = vkGetFenceStatus(fence->device(), *fence);
				if (res == VK_NOT_READY)
				{
					all_success = false;
					break;
				}
				// TODO check device lost
			}
			if(all_success)
			{
				// We can remove the inb
				inb = {};
				_previous_in_betweens.pop();
			}
			else
			{
				break;
			}
		}
		// Stack the in between
		if (!_in_between.fences.empty())
		{
			_previous_in_betweens.push(_in_between);
			_in_between = InBetween{};
		}
	}

	void LinearExecutor::submit()
	{
		VkSemaphore sem_to_wait = [&]() -> VkSemaphore {
			if (_in_between.semaphore)	return *_in_between.semaphore;
			return VK_NULL_HANDLE;
		}();

		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		std::shared_ptr<Fence> submit_fence = std::make_shared<Fence>(_app, name() + " Frame # " + std::to_string(_frame_index) + " Submit Fence");

		_in_between.next_cb = _command_buffer_to_submit;
		_in_between.fences.push_back(submit_fence);

		stackInBetween();

		VkCommandBuffer cb = *_command_buffer_to_submit;

		_in_between = InBetween{
			.prev_cb = _command_buffer_to_submit,
			.next_cb = nullptr,
			.fences = {submit_fence},
			.semaphore = std::make_shared<Semaphore>(_app, name() + " Frame # " + std::to_string(_frame_index) + " Submit Semaphore"),
		};

		VkSemaphore sem_to_signal = *_in_between.semaphore;

		const bool wait_on_sem = sem_to_wait != VK_NULL_HANDLE;

		VkSubmitInfo submission{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = wait_on_sem ? 1u : 0u,
			.pWaitSemaphores = wait_on_sem ? &sem_to_wait : nullptr,
			.pWaitDstStageMask = &wait_stage,
			.commandBufferCount = 1,
			.pCommandBuffers = &cb,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &sem_to_signal,
		};

		vkQueueSubmit(_app->queues().graphics, 1, &submission, *_in_between.fences[0]);

		_command_buffer_to_submit = nullptr;
	}

	void LinearExecutor::waitForAllCompletion(uint64_t timeout)
	{
		while (!_previous_in_betweens.empty())
		{
			InBetween& inb = _previous_in_betweens.front();
			for (auto& fence : inb.fences)
			{
				fence->wait(timeout);
			}
			_previous_in_betweens.pop();
		}
		waitForCurrentCompletion(timeout);
	}

	void LinearExecutor::waitForCurrentCompletion(uint64_t timeout)
	{
		if (!_in_between.fences.empty())
		{
			_in_between.fences.back()->wait(timeout);
		}
	}

	std::shared_ptr<CommandBuffer> LinearExecutor::getCommandBuffer()
	{
		return _command_buffer_to_submit;
	}
}