#include "LinearExecutor.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

namespace vkl
{
	LinearExecutor::LinearExecutor(CreateInfo const& ci) :
		Executor(ci.app ? ci.app : ci.window->application(), ci.name),
		_window(ci.window),
		_context(&_resources_state, nullptr)
	{
		if (ci.use_ImGui)
		{
			_render_gui = std::make_shared<ImguiCommand>(ImguiCommand::CI{
				.app = application(),
				.name = name() + ".RenderGui",
				.swapchain = _window->swapchain(),
				});
			declare(_render_gui);
		}
	}

	LinearExecutor::~LinearExecutor()
	{
		if (_render_gui)
		{
			ImGui_ImplVulkan_DestroyFontUploadObjects();
			ImGui_ImplVulkan_Shutdown();
			//ImGui_ImplVulkan_DestroyDeviceObjects();
		}
	}

	void LinearExecutor::declare(std::shared_ptr<Command> cmd)
	{
		_commands.emplace_back(std::move(cmd));
	}

	void LinearExecutor::declare(std::shared_ptr<ImageView> view)
	{
		_registered_images.emplace_back(std::move(view));
	}

	void LinearExecutor::declare(std::shared_ptr<Buffer> buffer)
	{
		_registered_buffers.emplace_back(std::move(buffer));
	}

	void LinearExecutor::declare(std::shared_ptr<Sampler> sampler)
	{
		_registered_samplers.emplace_back(std::move(sampler));
	}

	void LinearExecutor::preprocessCommands()
	{
		for (std::shared_ptr<Command>& cmd : _commands)
		{
			std::cout << "Initializing command " << cmd->name() << "\n";
			cmd->init();
		}
	}

	void LinearExecutor::preprocessResources()
	{

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

	void LinearExecutor::updateResources()
	{
		bool should_check_shaders = false;
		{
			std::chrono::time_point now = std::chrono::system_clock::now();
			std::chrono::duration diff = (now - _shader_check_time);
			if (diff > _shader_check_period)
			{
				should_check_shaders = true;
				_shader_check_time = now;
			}
		}
		UpdateContext update_context(UpdateContext::CI{
			.check_shaders = should_check_shaders,
			});

		if (_window->updateResources(update_context))
		{
			// TODO un register swapchain images
		}

		for (auto& image_view : _registered_images)
		{
			const bool invalidated = image_view->updateResource(update_context);
			if (invalidated)
			{
				const ImageRange ir{
					.image = *image_view->image()->instance(),
					.range = image_view->instance()->createInfo().subresourceRange,
				};
				const ResourceState2 state{
					._access = VK_ACCESS_2_NONE_KHR,
					._layout = image_view->image()->instance()->createInfo().initialLayout,
					._stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
				};
				_resources_state._image_states[ir] = state;

				image_view->removeInvalidationCallbacks(this);

				image_view->addInvalidationCallback(InvalidationCallback{
					.callback = [&]() {
						_resources_state._image_states.erase(ir);
					},
					.id = this,
					});
			}
		}

		for (auto& buffer : _registered_buffers)
		{
			const bool invalidated = buffer->updateResource(update_context);
			if (invalidated)
			{
				const ResourceState2 state{
					._access = VK_ACCESS_2_NONE_KHR,
					._stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
				};
				const VkBuffer b = *buffer->instance();
				_resources_state._buffer_states[b] = state;

				buffer->removeInvalidationCallbacks(this);

				buffer->addInvalidationCallback(InvalidationCallback{
					.callback = [&]() {
						_resources_state._buffer_states.erase(b);
					},
					.id = this,
					});
			}
		}

		for (auto& sampler : _registered_samplers)
		{
			const bool invalidated = sampler->updateResources(update_context);
		}

		for (auto& command : _commands)
		{
			const bool invalidated = command->updateResources(update_context);
		}
	}

	void LinearExecutor::beginFrame()
	{
		++_frame_index;
		//vkDeviceWaitIdle(device());
		std::shared_ptr frame_semaphore = std::make_shared<Semaphore>(_app, name() + " BeginFrame # " + std::to_string(_frame_index) + " Semaphore");
		_context.keppAlive(frame_semaphore);
		std::shared_ptr prev_semaphore = _in_between.semaphore;
		stackInBetween();
		_in_between = InBetween{
			.prev_cb = nullptr,
			.next_cb = nullptr,
			.fences = {std::make_shared<Fence>(_app, name() + " BeginFrame # " + std::to_string(_frame_index) + " Fence")},
			.semaphore = frame_semaphore,
		};
		_aquired = _window->aquireNextImage(_in_between.semaphore, _in_between.fences[0]);
		assert(_aquired.swap_index < _window->swapchainSize());
		_context.keppAlive(prev_semaphore);
		int _ = 0;
	}

	void LinearExecutor::preparePresentation(std::shared_ptr<ImageView> img_to_present, bool render_ImGui)
	{
		assert(!!_command_buffer_to_submit);
		std::shared_ptr<ImageView> blit_target = _window->view(_aquired.swap_index);

		execute((*_blit_to_present)(BlitImage::BI{
			.src = img_to_present,
			.dst = blit_target,
			}));

		if (render_ImGui && _render_gui)
		{
			_render_gui->setIndex(_aquired.swap_index);
			execute(_render_gui);
		}

		InputSynchronizationHelper synch(_context);
		Resource res{
			._image = blit_target,
			._begin_state = ResourceState2 {
				._access = VK_ACCESS_2_MEMORY_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				._stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, // Not sure about this one
			},
		};

		synch.addSynch(res);
		synch.record();
		synch.NotifyContext();
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

	void LinearExecutor::execute(Executable const& executable)
	{
		executable(_context);
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
		_in_between.dependecies = std::move(_context._objects_to_keep);
		_context._objects_to_keep.clear();

		// Removed finished in betweens
		while (!_previous_in_betweens.empty())
		{
			InBetween& inb = _previous_in_betweens.front();
			assert(!inb.fences.empty());
			bool all_success = true;
			for (const auto& fence : inb.fences)
			{
				const VkResult res = vkGetFenceStatus(fence->device(), *fence);
				if (res == VK_NOT_READY)
				{
					all_success = false;
					break;
				}
				// TODO check device lost
			}
			if (all_success)
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
		std::shared_ptr sem_to_wait = _in_between.semaphore;
		VkSemaphore vk_sem_to_wait = [&]() -> VkSemaphore {
			if (_in_between.semaphore)	return *_in_between.semaphore;
			return VK_NULL_HANDLE;
		}();

		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		std::shared_ptr<Fence> submit_fence = std::make_shared<Fence>(_app, name() + " Frame # " + std::to_string(_frame_index) + " Submit Fence");

		_in_between.next_cb = _command_buffer_to_submit;
		_in_between.fences.push_back(submit_fence);

		std::shared_ptr sem_to_signal = std::make_shared<Semaphore>(_app, name() + " Frame # " + std::to_string(_frame_index) + " Submit Semaphore");
		_context.keppAlive(sem_to_signal);
		stackInBetween();

		VkCommandBuffer cb = *_command_buffer_to_submit;


		_in_between = InBetween{
			.prev_cb = _command_buffer_to_submit,
			.next_cb = nullptr,
			.fences = {submit_fence},
			.semaphore = sem_to_signal,
		};
		_context.keppAlive(sem_to_wait);

		VkSemaphore vk_sem_to_signal = *_in_between.semaphore;

		const bool wait_on_sem = sem_to_wait != VK_NULL_HANDLE;

		VkSubmitInfo submission{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = wait_on_sem ? 1u : 0u,
			.pWaitSemaphores = wait_on_sem ? &vk_sem_to_wait : nullptr,
			.pWaitDstStageMask = &wait_stage,
			.commandBufferCount = 1,
			.pCommandBuffers = &cb,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &vk_sem_to_signal,
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
}