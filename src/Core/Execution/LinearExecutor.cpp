#include "LinearExecutor.hpp"

#include <Core/Rendering/DebugRenderer.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

namespace vkl
{
	LinearExecutor::LinearExecutor(CreateInfo const& ci) :
		Executor(Executor::CI{
			.app = ci.app ? ci.app : ci.window->application(), 
			.name = ci.name,
			.use_debug_renderer = ci.use_debug_renderer,
			.use_ray_tracing_pipeline = ci.use_ray_tracing,
		}),
		_window(ci.window),
		_staging_pool(StagingPool::CI{
			.app = application(),
			.name = name() + ".StagingPool",
			.allocator = application()->allocator(),
		}),
		_mounting_points(ci.mounting_points),
		_context(ExecutionContext::CI{
			.app = application(),
			.name = name() + ".exec_context",
			.resource_tid = 0,
			.staging_pool = &_staging_pool,
			.mounting_points = _mounting_points,
		})
	{
		(*_mounting_points)["ShaderLib"] = ENGINE_SRC_PATH "shaders/";

		if (ci.use_ImGui)
		{
			_render_gui = std::make_shared<ImguiCommand>(ImguiCommand::CI{
				.app = application(),
				.name = name() + ".RenderGui",
				.swapchain = _window->swapchain(),
				});
			_internal_resources += _render_gui;
		}

		buildCommonSetLayout();
		if (_use_debug_renderer)
		{
			createDebugRenderer();
		}
	}

	LinearExecutor::~LinearExecutor()
	{
		if (_render_gui)
		{
			//ImGui_ImplVulkan_DestroyDeviceObjects();
		}
	}

	void LinearExecutor::init()
	{
		_blit_to_present = std::make_shared<BlitImage>(BlitImage::CI{
			.app = _app,
			.name = name() + std::string(".BlitToPresent"),
		});
		_internal_resources += _blit_to_present;

		createCommonSet();
		_internal_resources += _common_descriptor_set;
		
		if (_render_gui)
		{

		}

		// TODO load debug renderer font here?
	}

	void LinearExecutor::updateResources(UpdateContext & context)
	{
		_common_definitions.update();

		if (_window->updateResources(context))
		{
			SwapchainInstance * swapchain = _window->swapchain()->instance().get();
		}

		_debug_renderer->updateResources(context);
		_internal_resources.update(context);
	}

	//ExecutionThread* LinearExecutor::beginTransferCommandBuffer(bool synch)
	//{
	//	std::shared_ptr<CommandBuffer>& cb = _command_buffer_to_submit;
	//	assert(!cb);
	//	cb = std::make_shared<CommandBuffer>(CommandBuffer::CI{
	//		.name = name() + ".Frame_" + std::to_string(_frame_index) + ".TransferCommandBuffer",
	//		.pool = _app->pools().transfer
	//	});
	//	cb->begin();
	//	_context.setCommandBuffer(cb);

	//	ExecutionThread * res;
	//}

	void LinearExecutor::AquireSwapchainImage()
	{
		++_frame_index;


		std::shared_ptr<Event> event = std::make_shared<Event>(application(), name() + ".AquireFrame_" + std::to_string(_frame_index), Event::Type::SwapchainAquire, true);

		VkWindow::AquireResult aquired = _window->aquireNextImage(event->signal_semaphore, event->signal_fence);
		event->swapchain = _window->swapchain()->instance();
		event->aquired_id = aquired.swap_index;
		assert(aquired.swap_index < _window->swapchainSize());
		_latest_aquire_event = event;
		_previous_events.push(std::move(event));
		
		int _ = 0;
	}

	void LinearExecutor::renderDebugIFP()
	{
		if (_use_debug_renderer)
		{
			_debug_renderer->execute(*_current_thread);
		}
	}

	void LinearExecutor::preparePresentation(std::shared_ptr<ImageView> img_to_present, bool render_ImGui)
	{
		assert(_latest_aquire_event);
		_latest_synch_cb->wait_semaphores.push_back(_latest_aquire_event->signal_semaphore);

		std::shared_ptr<ImageView> blit_target = _window->view(_latest_aquire_event->aquired_id);
		execute((*_blit_to_present)(BlitImage::BI{
			.src = img_to_present,
			.dst = blit_target,
		}));

		if (render_ImGui && _render_gui)
		{
			_render_gui->setIndex(_latest_aquire_event->aquired_id);
			execute(_render_gui);
		}

		SynchronizationHelper synch(_context);
		Resource res{
			._image = blit_target,
			._begin_state = ResourceState2 {
				.access = VK_ACCESS_2_MEMORY_READ_BIT,
				.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, // Not sure about this one
			},
		};

		synch.addSynch(res);
		synch.record();
	}

	void LinearExecutor::present()
	{
		//std::cout << "Present: " << std::endl;
		//std::cout << "Waiting on semaphore " << _latest_synch_cb->signal_semaphore->name() << std::endl;
		VkSemaphore sem_to_wait = _latest_synch_cb->signal_semaphore->handle();
		_window->present(1, &sem_to_wait);
	}

	void LinearExecutor::execute(Command& cmd)
	{
		_current_thread->execute(cmd);
	}

	void LinearExecutor::execute(std::shared_ptr<Command> cmd)
	{
		_current_thread->execute(cmd);
	}

	void LinearExecutor::execute(Executable const& executable)
	{
		_current_thread->execute(executable);
	}

	ExecutionThread * LinearExecutor::beginCommandBuffer(bool bind_common_set)
	{
		std::shared_ptr<CommandBuffer> cb = std::make_shared<CommandBuffer>(CommandBuffer::CI{
			.name = name() + ".CommandBuffer_" + std::to_string(_cb_count),
			.pool = _app->pools().graphics
		});
		++_cb_count;
		cb->begin();
		_context.setCommandBuffer(cb);


		std::shared_ptr<Event> event = std::make_shared<Event>(application(), cb->name(), Event::Type::CommandBuffer, true);
		event->cb = cb;
		event->queue = application()->queues().graphics;
		if (_latest_synch_cb && _latest_synch_cb->queue != event->queue) // No need to synch with a semaphore on the same queue
		{
			event->wait_semaphores.push_back(_latest_synch_cb->signal_semaphore);
		}
		_latest_synch_cb = event;
		
		assert(_current_thread == nullptr);
		ExecutionThread* res = new ExecutionThread(ExecutionThread::CI{
			.app = application(),
			.name = name() + ".ExecutionThread",
			.context = &_context,
		});
		_current_thread = res;
		if (bind_common_set)
		{
			bindSet(0, _common_descriptor_set, true, true, _use_rt_pipeline);
		}
		return res;
	}

	ExecutionThread* LinearExecutor::beginTransferCommandBuffer()
	{
		std::shared_ptr<CommandBuffer> cb = std::make_shared<CommandBuffer>(CommandBuffer::CI{
			.name = name() + ".TransferCommandBuffer_" + std::to_string(_cb_count),
			.pool = _app->pools().transfer,
		});
		++_cb_count;
		cb->begin();
		_context.setCommandBuffer(cb);

		std::shared_ptr<Event> event = std::make_shared<Event>(application(), cb->name(), Event::Type::CommandBuffer, true);
		event->cb = cb;
		event->queue = application()->queues().transfer;
		if (_latest_synch_cb && _latest_synch_cb->queue != event->queue) // No need to synch with a semaphore on the same queue
		{
			event->wait_semaphores.push_back(_latest_synch_cb->signal_semaphore);
		}
		_latest_synch_cb = event;

		assert(_current_thread == nullptr);
		ExecutionThread* res = new ExecutionThread(ExecutionThread::CI{
			.app = application(),
			.name = name() + ".ExecutionThread",
			.context = &_context,
		});
		_current_thread = res;
		return res;
	}

	void LinearExecutor::bindSet(uint32_t s, std::shared_ptr<DescriptorSetAndPool> const& set, bool bind_graphics, bool bind_compute, bool bind_rt)
	{
		_current_thread->bindSet(s, set, bind_graphics, bind_compute, bind_rt);
	}

	void LinearExecutor::endCommandBuffer(ExecutionThread * exec_thread, bool submit)
	{
		std::shared_ptr<CommandBuffer> cb = _context.getCommandBuffer();
		assert(exec_thread == _current_thread);
		delete exec_thread;
		_current_thread = nullptr;
		_context.setCommandBuffer(nullptr);

		cb->end();

		_pending_cbs.push_back(_latest_synch_cb);
		_latest_synch_cb->dependecies = std::move(_context.objectsToKeepAlive());
		_context.objectsToKeepAlive().clear();

		if (submit)
		{
			this->submit();
		}
	}

	void LinearExecutor::recyclePreviousEvents()
	{
		// TODO Really Recycle events resources (fences, semaphores)
		// Removed finished Events
		while (!_previous_events.empty())
		{
			std::shared_ptr<Event> event = _previous_events.front();
			
			const VkResult res = vkGetFenceStatus(device(), event->signal_fence->handle());
			if (res == VK_NOT_READY)
			{
				break;
			}
			assert(res == VK_SUCCESS);
			// TODO check device lost

			//if (event->finish_counter != 0)
			//{
			//	--event->finish_counter;
			//	break;
			//}

			
			//std::cout << "Event " << event->name() << " is Finished!" << std::endl;
			_previous_events.pop();
			
		}
	}

	void LinearExecutor::submit()
	{
		recyclePreviousEvents();
		
		std::vector<VkSemaphore> sem_to_wait;
		std::vector<VkPipelineStageFlags> stage_to_wait;
		for (size_t i = 0; i < _pending_cbs.size(); ++i)
		{
			const std::shared_ptr<Event> & pending = _pending_cbs[i];
			//std::cout << "Submit: " <<std::endl;
			//std::cout << "Signaling semaphore: " << pending->signal_semaphore->name() <<std::endl;
			//std::cout << "Waiting on semaphores: ";
			sem_to_wait.resize(pending->wait_semaphores.size());
			stage_to_wait.resize(pending->wait_semaphores.size());
			for (size_t s = 0; s < sem_to_wait.size(); ++s)
			{
				sem_to_wait[s] = pending->wait_semaphores[s]->handle();
				stage_to_wait[s] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				//std::cout << pending->wait_semaphores[s]->name() << ", ";
			}
			//std::cout << std::endl;
			VkCommandBuffer vk_cb = pending->cb->handle();
			VkSemaphore sem_to_signal = pending->signal_semaphore->handle();

			VkSubmitInfo submission{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = static_cast<uint32_t>(sem_to_wait.size()),
				.pWaitSemaphores = sem_to_wait.data(),
				.pWaitDstStageMask = stage_to_wait.data(),
				.commandBufferCount = 1,
				.pCommandBuffers = &vk_cb,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &sem_to_signal,
			};
			VkFence fence_to_signal = pending->signal_fence->handle();
			assert(pending->queue);
			VkResult res = vkQueueSubmit(pending->queue, 1, &submission, fence_to_signal);
			VK_CHECK(res, "Failed submission");
			_previous_events.push(pending);
		}
		_pending_cbs.clear();

	}

	void LinearExecutor::waitForAllCompletion(uint64_t timeout)
	{
		while (!_previous_events.empty())
		{
			Event& event = *_previous_events.front();
			event.signal_fence->wait(timeout);
			_previous_events.pop();
		}
	}
}