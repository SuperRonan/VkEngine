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
			ImGui_ImplVulkan_DestroyFontUploadObjects();
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
			ExecutionThread * thread = beginCommandBuffer(false);
			{
				ImGui_ImplVulkan_CreateFontsTexture(*_command_buffer_to_submit);
			}
			endCommandBufferAndSubmit(thread);
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

	void LinearExecutor::beginFrame()
	{
		++_frame_index;

		//vkDeviceWaitIdle(device());
		std::shared_ptr frame_semaphore = std::make_shared<Semaphore>(_app, name() + ".BeginFrame_" + std::to_string(_frame_index) + ".Semaphore");
		_context.keppAlive(frame_semaphore);
		std::shared_ptr prev_semaphore = _in_between.semaphore;
		stackInBetween();
		_in_between = InBetween{
			.prev_cb = nullptr,
			.next_cb = nullptr,
			.fences = {std::make_shared<Fence>(_app, name() + ".BeginFrame_" + std::to_string(_frame_index) + ".Fence")},
			.semaphore = frame_semaphore,
		};
		_aquired = _window->aquireNextImage(_in_between.semaphore, _in_between.fences[0]);
		assert(_aquired.swap_index < _window->swapchainSize());
		_context.keppAlive(prev_semaphore);
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
		VkSemaphore sem_to_wait = *_in_between.semaphore;
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
		std::shared_ptr<CommandBuffer>& cb = _command_buffer_to_submit;
		assert(!cb);
		cb = std::make_shared<CommandBuffer>(CommandBuffer::CI{
			.name = name() + ".Frame_" + std::to_string(_frame_index) + ".CommandBuffer",
			.pool = _app->pools().graphics
			});
		cb->begin();
		_context.setCommandBuffer(cb);

		if (bind_common_set)
		{
			if (_common_descriptor_set->instance()->exists())
			{
				_context.graphicsBoundSets().bind(0, _common_descriptor_set->instance());
				_context.computeBoundSets().bind(0, _common_descriptor_set->instance());
				if (_use_rt_pipeline)
				{
					_context.rayTracingBoundSets().bind(0, _common_descriptor_set->instance());
				}
			}
		}

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
		std::shared_ptr<DescriptorSetAndPoolInstance> inst = (set && set->instance()->exists()) ? set->instance() : nullptr;
		if (bind_graphics)
		{
			_context.graphicsBoundSets().bind(s, inst);
		}
		if (bind_compute)
		{
			_context.computeBoundSets().bind(s, inst);
		}
		if (bind_rt)
		{
			_context.rayTracingBoundSets().bind(s, inst);
		}
	}

	void LinearExecutor::endCommandBufferAndSubmit(ExecutionThread * exec_thread)
	{
		assert(exec_thread == _current_thread);
		delete _current_thread;
		_current_thread = nullptr;
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
		std::shared_ptr<Semaphore> sem_to_wait = _in_between.semaphore;
		VkSemaphore vk_sem_to_wait = [&]() -> VkSemaphore {
			if (_in_between.semaphore)	return *_in_between.semaphore;
			return VK_NULL_HANDLE;
		}();

		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		std::shared_ptr<Fence> submit_fence = std::make_shared<Fence>(_app, name() + ".Frame_" + std::to_string(_frame_index) + ".SubmitFence");

		_in_between.next_cb = _command_buffer_to_submit;
		_in_between.fences.push_back(submit_fence);

		std::shared_ptr sem_to_signal = std::make_shared<Semaphore>(_app, name() + ".Frame_" + std::to_string(_frame_index) + ".SubmitSemaphore");
		_context.keppAlive(sem_to_signal);
		stackInBetween();

		VkCommandBuffer cb = *_command_buffer_to_submit;


		_in_between = InBetween{
			.prev_cb = _command_buffer_to_submit,
			.next_cb = nullptr,
			.fences = {submit_fence},
			.semaphore = sem_to_signal,
		};
		if(sem_to_wait)
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