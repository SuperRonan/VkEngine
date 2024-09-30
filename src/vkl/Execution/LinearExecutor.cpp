#include <vkl/Execution/LinearExecutor.hpp>

#include <vkl/VkObjects/Queue.hpp>

#include <vkl/Rendering/DebugRenderer.hpp>

#include <vkl/Commands/PrebuiltTransferCommands.hpp>

#include <vkl/Execution/ExecutionStackReport.hpp>
#include <vkl/Execution/FramePerfReport.hpp>

#include <vkl/App/ImGuiApp.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

namespace vkl
{
	ExecutionThread::ExecutionThread(CreateInfo const& ci):
		ExecutionRecorder(ci.app, ci.name),
		_record_context(RecordContext::CI{
			.app = ci.app,
			.name = ci.name + ".RecordContext",
		}),
		_context(ci.context)
		//_synch(std::make_unique<SynchronizationHelper>())
	{
		_synch.reset(_context);
		reset();
	}

	bool ExecutionThread::useDeferredRecord() const
	{
		return _deferred_record || _current_render_pass_index != uint32_t(-1);
	}

	void ExecutionThread::releaseNodes()
	{
		for (auto& node : _nodes.getStorage())
		{
			node.node->finish();
		}
	}

	void ExecutionThread::recordEventNotRenderPass(uint32_t index, bool synch)
	{
		CommandEvent const& event = _commands[index];
		switch (event.type)
		{
		case CommandEvent::Type::ExecNode:
		{
			ExecNodeEvent & ene = _nodes.data()[event.index];

			_context->pushDebugLabel(ene.node->name(), true);

			if (synch)
			{
				_synch.reset(_context);
				_synch.commit(ene.node->resources());
				_synch.record();
			}

			ene.node->execute(*_context);
			ene.node->finish();

			_context->popDebugLabel();
		}
		break;
		case CommandEvent::Type::BindSet:
		{
			BindSetEvent & bse = _sets.data()[event.index];
			if (bse.info.bind_graphics)
			{
				_context->graphicsBoundSets().bind(bse.info.index, bse.info.set->instance());
			}
			if (bse.info.bind_compute)
			{
				_context->computeBoundSets().bind(bse.info.index, bse.info.set->instance());
			}
			if (bse.info.bind_rt)
			{
				_context->rayTracingBoundSets().bind(bse.info.index, bse.info.set->instance());
			}
		}
		break;
		case CommandEvent::Type::BeginRenderPass:
		case CommandEvent::Type::EndRenderPass:
		case CommandEvent::Type::NextSubPass:
		{
			assert(false);
		}
		break;
		case CommandEvent::Type::PushDebugLabel:
		case CommandEvent::Type::InsertDebugLabel:
		break;
		{
			DebugLabelEvent & dle = _debug_labels.data()[event.index];
			std::string_view sv = _strings.get(Range32u{ .begin = dle.string_index, .len = dle.string_len });
			if (event.type == CommandEvent::Type::PushDebugLabel)
			{
				_context->pushDebugLabel(sv, dle.color, dle.timestamp);
			}
			else
			{
				_context->insertDebugLabel(sv, dle.color);
			}
		}
		break;
		case CommandEvent::Type::PopDebugLabel:
		{
			_context->popDebugLabel();
		}
		break;
		}
	}

	void ExecutionThread::executeNode(std::shared_ptr<ExecutionNode> const& node)
	{
		assert(node->isInUse());
		if (useDeferredRecord())
		{
			ExecNodeEvent ex_ne{
				.node = node,
			};
			CommandEvent event{
				.type = CommandEvent::Type::ExecNode,
				.index = static_cast<uint32_t>(_nodes.pushBack(&ex_ne, 1)),
			};
			_commands.push_back(event);
			if (_current_render_pass_index != uint32_t(-1))
			{
				if (!_deferred_record)
				{
					_render_pass_resources += node->resources();
				}
			}
		}
		else
		{
			_context->pushDebugLabel(node->name(), true);
			_synch.reset(_context);
			_synch.commit(node->resources());
			_synch.record();
			node->execute(*_context);
			node->finish();
			_context->popDebugLabel();
		}
	}

	void ExecutionThread::record(Command& cmd)
	{
		std::shared_ptr<ExecutionNode> node = cmd.getExecutionNode(_record_context);
		executeNode(node);
	}

	void ExecutionThread::record(std::shared_ptr<Command> cmd)
	{
		record(*cmd);
	}

	void ExecutionThread::record(Executable const& executable)
	{
		std::shared_ptr<ExecutionNode> node = executable(_record_context);
		executeNode(node);
	}

	void ExecutionThread::bindSet(BindSetInfo const& info)
	{
		if (info.set)
		{
			info.set->waitForInstanceCreationIFN();
		}
		std::shared_ptr<DescriptorSetAndPoolInstance> inst = (info.set && info.set->instance()->exists()) ? info.set->instance() : nullptr;


		if (useDeferredRecord())
		{
			BindSetEvent bse{
				.info = info,
			};
			CommandEvent ce{
				.type = CommandEvent::Type::BindSet,
				.index = static_cast<uint32_t>(_sets.pushBack(&bse, 1)),
			};
			_commands.push_back(ce);
		}

		if (info.bind_graphics)
		{
			_record_context.graphicsBoundSets().bind(info.index, inst);
			if (!useDeferredRecord())
			{
				_context->graphicsBoundSets().bind(info.index, inst);
			}
		}
		if (info.bind_compute)
		{
			_record_context.computeBoundSets().bind(info.index, inst);
			if (!useDeferredRecord())
			{
				_context->computeBoundSets().bind(info.index, inst);
			}
		}
		if (info.bind_rt)
		{
			_record_context.rayTracingBoundSets().bind(info.index, inst);
			if (!useDeferredRecord())
			{
				_context->rayTracingBoundSets().bind(info.index, inst);
			}
		}
	}

	void ExecutionThread::clearDeferedLists()
	{
		releaseNodes();
		_nodes.clear();
		_sets.clear();
		_clear_values.clear();
		_begin_render_passes.clear();
		_debug_labels.clear();
		_strings.clear();
		_commands.clear();
	}

	void ExecutionThread::reset()
	{
		clearDeferedLists();
		_render_pass_resources.clear();
	}

	void ExecutionThread::beginRenderPass(RenderPassBeginInfo const& info, RenderPassBeginInfo::Flags flags)
	{
		assert(_current_render_pass_index == uint32_t(-1));
		BeginRenderPassEvent event{
			.info = info,
			.flags = flags,
		};
		_current_render_pass_index = _begin_render_passes.pushBack(&event, 1);
		_current_subpass_index = 0;
		RenderPassBeginInfo & _info = _begin_render_passes.data()[_current_render_pass_index].info;
		if (!_info.render_pass)
		{
			_info.render_pass = _info.framebuffer->renderPass();
		}
		uintptr_t & clear_values_index = reinterpret_cast<uintptr_t&>(_info.ptr_clear_values);
		clear_values_index = _clear_values.pushBack(info.ptr_clear_values, info.clear_value_count);
		
		CommandEvent ce{
			.type = CommandEvent::Type::BeginRenderPass,
			.index = _current_render_pass_index,
		};
		_commands.push_back(ce);

		if (!_deferred_record)
		{
			_render_pass_resources.clear();
			bool export_all_subpasses = !_render_pass_synch_subpass;
			_info.exportResources(_render_pass_resources, export_all_subpasses);
		}
		_record_context.beginRenderPass(_info.render_pass , _info.framebuffer);
	}

	bool ExecutionThread::getCurrentRenderingStatus(const RenderPassBeginInfo** info, SubpassInfo* subpass_info) const
	{
		bool res = (_current_render_pass_index != uint32_t(-1));
		if (res)
		{
			const BeginRenderPassEvent & r_event = _begin_render_passes.data()[_current_render_pass_index];
			const RenderPassBeginInfo & r = r_event.info;
			if (info)
			{
				*info = &r;
			}
			if (subpass_info)
			{
				if (_current_subpass_index == 0)
				{
					*subpass_info = SubpassInfo{
						.index = 0,
						.flags = r_event.flags,
					};
				}
				else
				{
					NOT_YET_IMPLEMENTED;
					*subpass_info = SubpassInfo{
						
					};
				}
			}
		}
		return res;
	}

	void ExecutionThread::nextSubPass(RenderPassBeginInfo::Flags flags)
	{
		assert(_current_render_pass_index != uint32_t(-1));
		BeginRenderPassEvent & rp_event = _begin_render_passes.data()[_current_render_pass_index];
		RenderPassBeginInfo& _info = rp_event.info;

		const uint32_t current_subpass_index = _record_context.getSubPassIndex();
		const uint32_t num_subpasses = _info.render_pass->getSubpasses().size32();
		assert(current_subpass_index < (num_subpasses - 1));

		if (_deferred_record)
		{
			CommandEvent event{
				.type = CommandEvent::Type::NextSubPass,
				.flags = flags,
			};
		}
		else 
		{
			if (_info.render_pass->handle()) 
			{
				_info.recordNextSubpass(*_context, flags);
			}
			else // Dynamic Rendering
			{
				NOT_YET_IMPLEMENTED;

				_info.recordEndSubpass(*_context);				

				_synch.reset(_context);

				_info.recordNextSubpass(*_context, flags);
			}
		}

		_record_context.nextSubPass();
	}

	void ExecutionThread::endRenderPass()
	{
		_record_context.endRenderPass();
		assert(_current_render_pass_index != uint32_t(-1));

		BeginRenderPassEvent& rp_event = _begin_render_passes.data()[_current_render_pass_index];
		RenderPassBeginInfo& _info = rp_event.info;

		if (_deferred_record)
		{
			_commands.push_back(CommandEvent{
				.type = CommandEvent::Type::EndRenderPass,
			});
		}
		else
		{
			if (_info.render_pass->handle())
			{
				_synch.reset(_context);
				_synch.commit(_render_pass_resources);
				_synch.record();
				_render_pass_resources.clear();
				
				_info.ptr_clear_values = _clear_values.data() + reinterpret_cast<uintptr_t>(_info.ptr_clear_values);
				_info.recordBegin(*_context, rp_event.flags | RenderPassBeginInfo::Flags::ContentsInline);

				uint32_t index = _current_render_pass_index + 1;
				while(index < _commands.size32())
				{
					CommandEvent & ce = _commands[index];
					if (ce.type == CommandEvent::Type::EndRenderPass)
					{
						break;
					}
					if (ce.type == CommandEvent::Type::NextSubPass)
					{
						_info.recordNextSubpass(*_context, ce.flags);
					}
					else
					{
						recordEventNotRenderPass(index, false);
					}
					++index;
				}

				_info.recordEnd(*_context);
			}
			else
			{
				NOT_YET_IMPLEMENTED;
			}
			clearDeferedLists();
		}

		_current_render_pass_index = uint32_t(-1);
	}

	void ExecutionThread::pushDebugLabel(std::string_view const& label, vec4 const& color, bool timestamp)
	{
		if (useDeferredRecord())
		{
			DebugLabelEvent dle{
				.string_index = static_cast<uint32_t>(_strings.pushBack(label)),
				.string_len = static_cast<uint32_t>(label.size()),
				.color = color,
				.timestamp = timestamp,
			};
			CommandEvent event{
				.type = CommandEvent::Type::PushDebugLabel,
				.index = static_cast<uint32_t>(_debug_labels.pushBack(&dle, 1)),
			};
			_commands.push_back(event);
		}
		else
		{
			_context->pushDebugLabel(label, color, timestamp);
		}
	}

	void ExecutionThread::popDebugLabel()
	{
		if (useDeferredRecord())
		{
			CommandEvent event{
				.type = CommandEvent::Type::PopDebugLabel,
			};
			_commands.push_back(event);
		}
		else
		{
			_context->popDebugLabel();
		}
	}

	void ExecutionThread::insertDebugLabel(std::string_view const& label, vec4 const& color)
	{
		if (useDeferredRecord())
		{
			DebugLabelEvent dle{
				.string_index = static_cast<uint32_t>(_strings.pushBack(label)),
				.string_len = static_cast<uint32_t>(label.size()),
				.color = color,
			};
			CommandEvent event{
				.type = CommandEvent::Type::InsertDebugLabel,
				.index = static_cast<uint32_t>(_debug_labels.pushBack(&dle, 1)),
			};
			_commands.push_back(event);
		}
		else
		{
			_context->insertDebugLabel(label, color);
		}
	}

	void ExecutionThread::recordCommands()
	{
		// Assume the command pool mutex is owned
		uint32_t index;
	}



	struct CommandBufferSubmission : public VkObject
	{
		std::shared_ptr<CommandBuffer> cb = nullptr;
		Queue * queue = nullptr;

		MyVector<std::shared_ptr<Semaphore>> wait_semaphores = {};
		// Can be multiple because VkSemaphore can be consumed only ONCE (WTF who designed that???)
		MyVector<std::shared_ptr<Semaphore>> signal_semaphores = {};

		std::shared_ptr<Fence> signal_fence = nullptr;

		MyVector<std::shared_ptr<VkObject>> dependecies = {};
		MyVector<CompletionCallback> completion_callbacks = {};

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			bool create_fence = false;
			uint32_t create_semaphore = 0;
		};
		using CI = CreateInfo;

		CommandBufferSubmission(CreateInfo const& ci) :
			VkObject(ci.app, ci.name)
		{
			if (ci.create_fence)
			{
				signal_fence = std::make_shared<Fence>(application(), this->name() + ".SignalFence");
			}
			if (ci.create_semaphore)
			{
				signal_semaphores.resize(ci.create_semaphore);
				for (uint32_t i = 0; i < ci.create_semaphore; ++i)
				{
					signal_semaphores[i] = std::make_shared<Semaphore>(application(), this->name() + ".SignalSemaphore_"s + std::to_string(i));
				}
			}
		}

		virtual ~CommandBufferSubmission() override
		{
			wait_semaphores.clear();
			signal_semaphores.clear();
			
			signal_fence.reset();
			VKL_BREAKPOINT_HANDLE;
		}

		void finish(VkResult result, MyVector<std::shared_ptr<CommandBuffer>> * cb_trash_can = nullptr)
		{
			for (auto& callback : completion_callbacks)
			{
				callback(static_cast<int>(result));
			}
			completion_callbacks.clear();
			dependecies.clear();
			if (cb_trash_can)
			{
				cb_trash_can->push_back(std::move(cb));
			}
			else
			{
				cb.reset();
			}
		}
	};

	struct SwapchainEvent : public VkObject
	{
		std::shared_ptr<SwapchainInstance> swapchain = nullptr;
		uint32_t index = 0;

		std::shared_ptr<Semaphore> aquire_signal_semaphore = nullptr;
		std::shared_ptr<Fence> aquire_signal_fence = nullptr;

		MyVector<std::shared_ptr<Semaphore>> present_wait_semaphores = {};
		std::shared_ptr<Fence> present_signal_fence = nullptr;

		// present_queue == nullptr -> not presented yet
		Queue * present_queue = nullptr;
		uint64_t present_id = 0;

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<SwapchainInstance> swapchain = nullptr;
			uint32_t index = 0;
			bool create_aquire_semaphore = false;
			bool create_aquire_fence = false;
			bool create_present_fence = false;
		};
		using CI = CreateInfo;

		SwapchainEvent(CreateInfo const& ci):
			VkObject(ci.app, ci.name),
			swapchain(ci.swapchain),
			index(ci.index), 
			aquire_signal_semaphore(ci.create_aquire_semaphore ? std::make_shared<Semaphore>(application(), this->name() + ".AquireSignalSemaphore") : nullptr),
			aquire_signal_fence(ci.create_aquire_fence ? std::make_shared<Fence>(application(), this->name() + ".AquireSignalFence") : nullptr),
			present_signal_fence(ci.create_present_fence ? std::make_shared<Fence>(application(), this->name() + ".PresentSignalFence") : nullptr)
		{}

		virtual ~SwapchainEvent() override
		{
			VKL_BREAKPOINT_HANDLE;
		}
	};

	LinearExecutor::LinearExecutor(CreateInfo const& ci) :
		Executor(Executor::CI{
			.app = ci.app ? ci.app : ci.window->application(), 
			.name = ci.name,
			.common_definitions = ci.common_definitions,
			.use_debug_renderer = ci.use_debug_renderer,
			.use_ray_tracing_pipeline = ci.use_ray_tracing,
		}),
		_window(ci.window),
		_context(ExecutionContext::CI{
			.app = application(),
			.name = name() + ".exec_context",
			.resource_tid = 0,
		})
	{
		// TODO better later
		_main_queue = application()->queuesByFamily().front().front();
		_present_queue = _main_queue;

		if (ci.use_ImGui)
		{
			_render_gui = std::make_shared<ImguiCommand>(ImguiCommand::CI{
				.app = application(),
				.name = name() + ".RenderGui",
				.swapchain = _window->swapchain(),
				.queue = _main_queue,
			});
			_internal_resources += _render_gui;

			_frame_end_fences.resize(8);
		}

		buildCommonSetLayout();
		if (_use_debug_renderer)
		{
			createDebugRenderer();
		}

		if (_window && !useSpecificPresentSignalFence())
		{
			_window->swapchain()->setInvalidationCallback(Callback{
				.callback = [this]() {
					// Could also copy the _previous_swapchain_events somewhere and delete them later
					waitOnSwapchainCompletion();
				},
				.id = this,
			});
		}

		_execution_thread = std::make_unique<ExecutionThread>(ExecutionThread::CI{
			.app = application(),
			.name = name() + ".ExecutionThread",
			.context = &_context,
		});
	}

	LinearExecutor::~LinearExecutor()
	{
		vkDeviceWaitIdle(device());
		if (_window && !useSpecificPresentSignalFence())
		{
			_window->swapchain()->removeInvalidationCallback(this);
		}
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
		if (_window->updateResources(context))
		{
			SwapchainInstance * swapchain = _window->swapchain()->instance().get();
		}

		if (_debug_renderer)
		{
			_debug_renderer->updateResources(context);
		}
		_internal_resources.update(context);

		_frame_perf_report_pool_mutex.lock();
		{
			for (size_t i = 0; i < _frame_perf_report_pool.size(); ++i)
			{
				_frame_perf_report_pool[i]->timestamp_query_pool->updateResources(context);
			}
		}
		_frame_perf_report_pool_mutex.unlock();
	}

	void LinearExecutor::beginFrame(bool capture_report)
	{
		++_frame_index;
		
		_context._timestamp_query_count = 0;
		_context._tick_tock.tick();
		if (capture_report)
		{
			_frame_perf_report_pool_mutex.lock();
			{
				if (!_frame_perf_report_pool.empty())
				{
					_current_frame_report = std::move(_frame_perf_report_pool.back());
					_frame_perf_report_pool.pop_back();
				}	
			}
			_frame_perf_report_pool_mutex.unlock();

			if (!_current_frame_report)
			{
				_current_frame_report = std::make_shared<FramePerfReport>();
				_current_frame_report->report = std::make_shared<ExecutionStackReport>(ExecutionStackReport::CI{
					.app = application(),
					.name = name() + ".ExecutionStackReport",
				});
				_current_frame_report->timestamp_query_pool = std::make_shared<QueryPool>(QueryPool::CI{
					.app = application(),
					.name = name() + "TimestampQueryPool",
					.type = VK_QUERY_TYPE_TIMESTAMP,
					.count = &_timestamp_query_pool_capacity,
					.hold_instance = true,
				});
				_current_frame_report->timestamp_query_pool->createInstance();
				_current_frame_report->emit_mt = true;
			}

			_current_frame_report->clear();
			_current_frame_report->push();
			_context._stack_report = _current_frame_report->report;
			_context._timestamp_query_pool = _current_frame_report->timestamp_query_pool->instance();
		}
		
	}

	void LinearExecutor::endFrame()
	{
		if (_current_frame_report)
		{
			_current_frame_report->query_count = _context._timestamp_query_count;
			_current_frame_report->pop();
			if (_pending_frame_report)
			{
				std::unique_lock lock(_frame_perf_report_pool_mutex);
				_frame_perf_report_pool.push_back(std::move(_pending_frame_report));
			}
			_pending_frame_report = std::move(_current_frame_report);
			_context._timestamp_query_pool = nullptr;
			_context._stack_report = nullptr;
		}
		_timestamp_query_pool_capacity = std::max(_timestamp_query_pool_capacity, _context._timestamp_query_count * 2);
	}

	//ExecutionRecorder* LinearExecutor::beginTransferCommandBuffer(bool synch)
	//{
	//	std::shared_ptr<CommandBuffer>& cb = _command_buffer_to_submit;
	//	assert(!cb);
	//	cb = std::make_shared<CommandBuffer>(CommandBuffer::CI{
	//		.name = name() + ".Frame_" + std::to_string(_frame_index) + ".TransferCommandBuffer",
	//		.pool = _app->pools().transfer
	//	});
	//	cb->begin();
	//	_context.setCommandBuffer(cb);

	//	ExecutionRecorder * res;
	//}

	bool LinearExecutor::useSpecificPresentSignalFence() const
	{
		return application()->availableFeatures().swapchain_maintenance1_ext.swapchainMaintenance1 != VK_FALSE;
	}

	void LinearExecutor::aquireSwapchainImage()
	{
		const bool use_specific_present_signal_fence = useSpecificPresentSignalFence();
		std::shared_ptr<SwapchainEvent> & event = _latest_swapchain_event;
		event = std::make_shared<SwapchainEvent>(SwapchainEvent::CI{
			.app = application(), 
			.name = name() + ".AquireFrame_" + std::to_string(_frame_index), 
			.create_aquire_semaphore = true,
			.create_aquire_fence = true, 
			.create_present_fence = use_specific_present_signal_fence,
		});

		VkWindow::AquireResult aquired = _window->aquireNextImage(event->aquire_signal_semaphore, event->aquire_signal_fence);
		event->swapchain = _window->swapchain()->instance();
		event->index = aquired.swap_index;

		if(false)
		{
			std::unique_lock lock(g_mutex);
			std::cout << "Aquired Swapchain index: " << event->index << std::endl;
		}

		assert(aquired.swap_index < _window->swapchainSize());

		if (!use_specific_present_signal_fence)
		{
			_swapchain_mutex.lock();
			// find the most recent present event with the same swapchain and index
			auto it = _previous_swapchain_events.rbegin();
			while (it != _previous_swapchain_events.rend())
			{
				std::shared_ptr<SwapchainEvent> & e = *it;
				if ((e->swapchain == event->swapchain) && (e->index == event->index) && !e->present_signal_fence)
				{
					break;
				}
				else
				{
					++it;
				}
			}
			if (it != _previous_swapchain_events.rend())
			{
				std::shared_ptr<SwapchainEvent>& e = *it;
				assert(e->present_queue != nullptr);
				if (!e->present_signal_fence)
				{
					e->present_signal_fence = event->aquire_signal_fence;
				}
				else
				{
					VKL_BREAKPOINT_HANDLE;
				}
			}
			else
			{
				VKL_BREAKPOINT_HANDLE;
			}
			_swapchain_mutex.unlock();
		}

		VKL_BREAKPOINT_HANDLE;
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
		assert(_latest_swapchain_event);
		assert(_latest_swapchain_event->present_queue == nullptr);
		_latest_synch_cb->wait_semaphores.push_back(_latest_swapchain_event->aquire_signal_semaphore);

		if (_frame_end_fences)
		{
			const size_t fence_index = _frame_index % _frame_end_fences.size();
			std::shared_ptr<Fence>& fence = _frame_end_fences[fence_index];
			if (fence)
			{
				fence->wait();
			}

			fence = _latest_synch_cb->signal_fence;
		}

		{
			std::shared_ptr<ImageView> blit_target = _latest_swapchain_event->swapchain->views()[_latest_swapchain_event->index];
			execute((*_blit_to_present)(BlitImage::BI{
				.src = img_to_present,
				.dst = blit_target,
			}));
			
			if (render_ImGui && _render_gui && _latest_swapchain_event->swapchain == _window->swapchain()->instance())
			{
				execute(_render_gui->with(ImguiCommand::ExecutionInfo{.index = _latest_swapchain_event->index}));
			}

			InlineSynchronizeImageView(_context, blit_target->instance(), ResourceState2{
				.access = VK_ACCESS_2_MEMORY_READ_BIT,
				.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, // Not sure about this one
				.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			});
		}

	}

	void LinearExecutor::present()
	{
		static MyVector<VkSemaphore> wait_semaphores;
		static MyVector<VkSwapchainKHR> swapchains;
		static MyVector<uint32_t> indices;
		static MyVector<VkResult> results;
		static MyVector<VkFence> fences;
		static MyVector<uint64_t> present_ids;

		const bool use_signal_fence = useSpecificPresentSignalFence();
		const bool use_present_id = application()->availableFeatures().present_id_khr.presentId;
		
		_latest_swapchain_event->present_wait_semaphores.push_back(_latest_synch_cb->signal_semaphores[0]);

		wait_semaphores.resize(_latest_swapchain_event->present_wait_semaphores.size());
		for (size_t i = 0; i < _latest_swapchain_event->present_wait_semaphores.size(); ++i)
		{
			wait_semaphores[i] = _latest_swapchain_event->present_wait_semaphores[i]->handle();
		}

		std::array<std::shared_ptr<SwapchainEvent>, 1> targets = {_latest_swapchain_event};

		const uint32_t n = targets.size();
		assert(n > 0);
		swapchains.resize(n);
		indices.resize(n);
		results.resize(n);
		if (use_signal_fence)
		{
			fences.resize(n);
		}
		if (use_present_id)
		{
			present_ids.resize(n);
		}

		
		for (uint32_t i = 0; i < n; ++i)
		{
			swapchains[i] = targets[i]->swapchain->handle();
			indices[i] = targets[i]->index;
			results[i] = VK_RESULT_MAX_ENUM;
			if (use_signal_fence)
			{
				assert(targets[i]->present_signal_fence);
				fences[i] = targets[i]->present_signal_fence->handle();
			}
			targets[i]->present_queue = _present_queue.get();
			if (use_present_id)
			{
				targets[i]->present_id = targets[i]->swapchain->getNextPresentId();
				present_ids[i] = targets[i]->present_id;
			}
		}

		VkPresentInfoKHR presentation{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = wait_semaphores.size32(),
			.pWaitSemaphores = wait_semaphores.data(),
			.swapchainCount = swapchains.size32(),
			.pSwapchains = swapchains.data(),
			.pImageIndices = indices.data(),
			.pResults = results.data(),
		};
		pNextChain chain = &presentation;
		VkSwapchainPresentFenceInfoEXT present_fence;
		if (use_signal_fence)
		{
			present_fence = VkSwapchainPresentFenceInfoEXT{
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
				.pNext = nullptr,
				.swapchainCount = n,
				.pFences = fences.data(),
			};
			chain += &present_fence;
		}
		VkPresentIdKHR vk_present_id;
		if (use_present_id)
		{
			vk_present_id = VkPresentIdKHR{
				.sType = VK_STRUCTURE_TYPE_PRESENT_ID_KHR,
				.pNext = nullptr,
				.swapchainCount = n,
				.pPresentIds = present_ids.data(),
			};
			chain += &vk_present_id;
		}

		assert(_present_queue);
		_present_queue->mutex().lock();
		VkResult present_res = vkQueuePresentKHR(_present_queue->handle(), &presentation);
		_present_queue->mutex().unlock();
		
		for (uint32_t i = 0; i < n; ++i)
		{
			// TODO keep a list of windows
			if (i == 0)
			{
				_window->notifyPresentResult(results[i]);
			}
		}

		_swapchain_mutex.lock();
		_previous_swapchain_events.push_back(std::move(_latest_swapchain_event));
		_swapchain_mutex.unlock();
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

	void LinearExecutor::performSynchTransfers(UpdateContext& update_context, bool consume_asynch, TransferBudget budget)
	{
		ExecutionThread* upload_thread = beginCommandBuffer();
		UploadResources &uploader = application()->getPrebuiltTransferCommands().upload_resources;
		static thread_local ResourcesToUpload upload_list;
		upload_list = update_context.resourcesToUpload();
		if (consume_asynch && update_context.uploadQueue())
		{

			const size_t total_budget_bytes = budget.bytes;
			const size_t min_asynch_budget_bytes = 1'000'000;
			size_t synch_upload_cost = upload_list.getSize();
			TransferBudget asynch_upload_budget{
				.instances = budget.instances,
			};
			if (synch_upload_cost > (total_budget_bytes - min_asynch_budget_bytes))
			{
				asynch_upload_budget.bytes = min_asynch_budget_bytes;
			}
			else
			{
				asynch_upload_budget.bytes = total_budget_bytes - synch_upload_cost;
			}
			ResourcesToUpload asynch_list = update_context.uploadQueue()->consume(asynch_upload_budget);
			upload_list += std::move(asynch_list);
		}
		
		
		(*upload_thread)(uploader.with(UploadResources::UI{
			.upload_list = std::move(upload_list),
		}));
		upload_list.clear();
		
		endCommandBuffer(upload_thread);
	}

	void LinearExecutor::performAsynchMipsCompute(MipMapComputeQueue& mips_queue, TransferBudget budget)
	{
		auto mips_list = mips_queue.consume(budget);

		if (!mips_list.empty())
		{
			ComputeMips & compute_mips = application()->getPrebuiltTransferCommands().compute_mips;
			_current_thread->execute(compute_mips.with(ComputeMips::ExecInfo{
				.targets = mips_list,
			}));
		}
	}

	ExecutionThread* LinearExecutor::beginCommandBuffer(bool bind_common_set)
	{
		std::shared_ptr<CommandPool> pool = _app->commandPools().front();
		pool->mutex().lock();
		std::shared_ptr<CommandBuffer> cb = std::make_shared<CommandBuffer>(CommandBuffer::CI{
			.name = name() + ".CommandBuffer_" + std::to_string(_cb_count),
			.pool = pool,
		});
		pool->mutex().unlock();
		++_cb_count;
		cb->begin();
		cb->pool()->mutex().lock();
		_context.setCommandBuffer(cb);

		std::shared_ptr<CommandBufferSubmission> event = std::make_shared<CommandBufferSubmission>(CommandBufferSubmission::CI{
			.app = application(), 
			.name = cb->name(), 
			.create_fence = true, 
			.create_semaphore = 1,
		});
		event->cb = cb;
		event->queue = _main_queue.get();
		if (_latest_synch_cb && _latest_synch_cb->queue != event->queue) // No need to synch with a semaphore on the same queue
		{
			event->wait_semaphores.push_back(_latest_synch_cb->signal_semaphores[0]);
		}
		_latest_synch_cb = event;
		
		assert(_current_thread == nullptr);
		_current_thread = _execution_thread.get();

		if (_current_frame_report)
		{
			_current_frame_report->push();
			_current_frame_report->max_timestamp_bits = std::max(_current_frame_report->max_timestamp_bits, event->queue->properties().timestampValidBits);
			if (!_current_frame_report->query_reseted)
			{
				vkCmdResetQueryPool(_context.getCommandBuffer()->handle(), _current_frame_report->timestamp_query_pool->instance()->handle(), 0, _current_frame_report->timestamp_query_pool->instance()->count());
				_current_frame_report->query_reseted = true;
			}
			_current_thread->pushDebugLabel(cb->name(), true);
		}

		if (bind_common_set)
		{
			_current_thread->bindSet(BindSetInfo{
				.index = 0,
				.set = _common_descriptor_set, 
				.bind_graphics = true,
				.bind_compute = true,
				.bind_rt = _use_rt_pipeline,
			});
		}
		return _current_thread;
	}

	//ExecutionThread* LinearExecutor::beginTransferCommandBuffer()
	//{
	//	std::shared_ptr<CommandBuffer> cb = std::make_shared<CommandBuffer>(CommandBuffer::CI{
	//		.name = name() + ".TransferCommandBuffer_" + std::to_string(_cb_count),
	//		.pool = _app->pools().transfer,
	//	});
	//	++_cb_count;
	//	cb->begin();
	//	_context.setCommandBuffer(cb);

	//	std::shared_ptr<Event> event = std::make_shared<Event>(application(), cb->name(), Event::Type::CommandBuffer, true);
	//	event->cb = cb;
	//	event->queue = application()->queues().transfer;
	//	if (_latest_synch_cb && _latest_synch_cb->queue != event->queue) // No need to synch with a semaphore on the same queue
	//	{
	//		event->wait_semaphores.push_back(_latest_synch_cb->signal_semaphore);
	//	}
	//	_latest_synch_cb = event;

	//	assert(_current_thread == nullptr);
	//	ExecutionRecorder* res = new ExecutionRecorder(ExecutionRecorder::CI{
	//		.app = application(),
	//		.name = name() + ".ExecutionRecorder",
	//		.context = &_context,
	//	});
	//	_current_thread = res;
	//	return res;
	//}

	void LinearExecutor::bindSet(BindSetInfo const& info)
	{
		_current_thread->bindSet(info);
	}

	void LinearExecutor::endCommandBuffer(ExecutionThread* exec_thread, bool submit)
	{
		if (_current_frame_report)
		{
			std::shared_ptr<FramePerfReport> frame_report = _current_frame_report;
			_context.addCompletionCallback([frame_report](int result)
			{
				frame_report->pop();
			});
			exec_thread->popDebugLabel();
		}
		assert(_context._debug_stack_depth == 0);
		std::shared_ptr<CommandBuffer> cb = _context.getCommandBuffer();
		assert(exec_thread == _current_thread);
		_current_thread = nullptr;
		_context.setCommandBuffer(nullptr);
		_execution_thread->reset();

		cb->end();
		cb->pool()->mutex().unlock();

		_pending_cbs.push_back(_latest_synch_cb);
		_latest_synch_cb->dependecies = std::move(_context.objectsToKeepAlive());
		_latest_synch_cb->completion_callbacks = std::move(_context.completionCallbacks());
		_context.objectsToKeepAlive().clear();

		if (submit)
		{
			this->submit();
		}
	}

	void LinearExecutor::recyclePreviousEvents()
	{
		// TODO Really Recycle events resources
		// Unfortunately, we can't destroy a command buffer in one thread and record another command buffer from the same pool in another thread
		// I personally think it should not be the case (vkCmd* write to a command buffer, not a command pool (but writing to the command buffer probably requires interactive with the pool, to get more memory))
		// But it is a it is.
		// I think it implies that two concurent threads can't record two command buffers from the same pool.
		// Possible solutions:
		// (1): lock the pool mutex at each call to vkCmd*
		// What a pain, won't do that + probably quite ineficient 
		// (2): Use multiple pools and swap between them
		// Probably the right solution on the long run
		// (3): Keep the to-be-destroyed CommandBuffers in a trash can and destroy them on the main thread
		// Going with solution (3) for now

		const bool use_mt = application()->threadPool().isMultiThreaded();
		// It appears the gain of using mt is not that great, but present (solution (3) also slows things down a bit)
		auto recycle = [this, use_mt]()
		{
			MyVector<std::shared_ptr<CommandBuffer>> * cb_trash_can = nullptr;
			if (use_mt)
			{
				cb_trash_can = &_trash_cbs;
			}
			const bool cb_break_on_first_stall = false;
			// It seems there is a bug with this to true, leading to an accumulation of previous swapchain event
			const bool swapchain_break_on_first_stall = false;
			_cb_mutex.lock();
			{
				if (cb_break_on_first_stall)
				{
					while (!_previous_cbs.empty())
					{
						std::shared_ptr<CommandBufferSubmission>& submitted_cb = _previous_cbs.front();
						VkResult status = submitted_cb->signal_fence->getStatus();
						if (status == VK_NOT_READY)
						{
							break;
						}
						else
						{
							assert(status == VK_SUCCESS);
							submitted_cb->finish(status, cb_trash_can);
							_previous_cbs.pop_front();
						}
					}
				}
				else
				{
					auto it = _previous_cbs.begin();
					while(it != _previous_cbs.end())
					{
						std::shared_ptr<CommandBufferSubmission> & submitted_cb = *it;
						VkResult status = submitted_cb->signal_fence->getStatus();
						if (status == VK_NOT_READY)
						{
							++it;
						}
						else
						{
							assert(status == VK_SUCCESS);
							submitted_cb->finish(status, cb_trash_can);
							it = _previous_cbs.erase(it);
						}
					}
				}
			}
			_cb_mutex.unlock();

			_swapchain_mutex.lock();
			{
				auto getStatus = [&](std::shared_ptr<SwapchainEvent>& e)
				{
					VkResult res = VK_NOT_READY;
					if (e->present_queue)
					{
						if (e->present_signal_fence)
						{
							res = e->present_signal_fence->getStatus();
						}
						else if (application()->availableFeatures().present_wait_khr.presentWait)
						{
							res = application()->extFunctions()._vkWaitForPresentKHR(device(), e->swapchain->handle(), e->present_id, 0);
						}
						VkResult aquire_status = e->aquire_signal_fence->getStatus();
						if (res == VK_SUCCESS && aquire_status != VK_SUCCESS)
						{
							VKL_BREAKPOINT_HANDLE;
						}
					}
					return res;
				};
				if (swapchain_break_on_first_stall)
				{
					while (!_previous_swapchain_events.empty())
					{
						std::shared_ptr<SwapchainEvent>& e = _previous_swapchain_events.front();
						VkResult status = getStatus(e);
						if (status == VK_NOT_READY)
						{
							break;
						}
						else
						{
							assert(status == VK_SUCCESS);
							_previous_swapchain_events.pop_front();
						}
					}
				}
				else
				{
					auto it = _previous_swapchain_events.begin();
					while (it != _previous_swapchain_events.end())
					{
						std::shared_ptr<SwapchainEvent>& e = *it;
						VkResult status = getStatus(e);
						if (status == VK_NOT_READY)
						{
							++it;
						}
						else
						{
							assert(status == VK_SUCCESS);
							it = _previous_swapchain_events.erase(it);
						}
					}
				}
			}
			_swapchain_mutex.unlock();
		};
		
		if (use_mt)
		{
			if (_previous_recycle_task)
			{
				_previous_recycle_task->waitIFN();
			}

			_trash_cbs.clear();
			
			auto lambda = [recycle]()
			{
				recycle();
				return AsynchTask::ReturnType {
					.success = true,
				};
			};
			_previous_recycle_task = std::make_shared<AsynchTask>(AsynchTask::CI{
				.name = name() + ".Recycle()",
				.verbosity = 0,
				.priority = TaskPriority::ASAP(),
				.lambda = lambda,
				
			});
			application()->threadPool().pushTask(_previous_recycle_task);
		}
		else
		{
			recycle();
		}
	}

	void LinearExecutor::submit()
	{
		recyclePreviousEvents();
		
		_cb_mutex.lock();
		static thread_local MyVector<VkSemaphore> vk_semaphores;
		static thread_local MyVector<VkPipelineStageFlags> stage_to_wait;
		// TODO batch all submissions
		for (size_t i = 0; i < _pending_cbs.size(); ++i)
		{
			const std::shared_ptr<CommandBufferSubmission> & pending = _pending_cbs[i];
			//std::cout << "Submit: " <<std::endl;
			//std::cout << "Signaling semaphore: " << pending->signal_semaphore->name() <<std::endl;
			//std::cout << "Waiting on semaphores: ";
			const uint32_t wait_semaphore_count = pending->wait_semaphores.size32();
			const uint32_t signal_semaphore_count = pending->signal_semaphores.size32();
			vk_semaphores.resize(wait_semaphore_count + signal_semaphore_count);
			stage_to_wait.resize(pending->wait_semaphores.size());
			for (uint32_t s = 0; s < wait_semaphore_count; ++s)
			{
				vk_semaphores[s] = pending->wait_semaphores[s]->handle();
				stage_to_wait[s] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				//std::cout << pending->wait_semaphores[s]->name() << ", ";
			}
			for (uint32_t i = 0; i < signal_semaphore_count; ++i)
			{
				vk_semaphores[i + wait_semaphore_count] = pending->signal_semaphores[i]->handle();
			}
			//std::cout << std::endl;
			VkCommandBuffer vk_cb = pending->cb->handle();
			VkSemaphore * sem_to_wait = vk_semaphores.data();
			VkSemaphore * sem_to_signal = vk_semaphores.data() + wait_semaphore_count;

			VkSubmitInfo submission{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = wait_semaphore_count,
				.pWaitSemaphores = sem_to_wait,
				.pWaitDstStageMask = stage_to_wait.data(),
				.commandBufferCount = 1,
				.pCommandBuffers = &vk_cb,
				.signalSemaphoreCount = signal_semaphore_count,
				.pSignalSemaphores = sem_to_signal,
			};
			VkFence fence_to_signal = pending->signal_fence->handle();
			assert(pending->queue);
			pending->queue->mutex().lock();
			VkResult res = vkQueueSubmit(pending->queue->handle(), 1, &submission, fence_to_signal);
			pending->queue->mutex().unlock();
			VK_CHECK(res, "Failed submission");
			//vkDeviceWaitIdle(device());
			_previous_cbs.push_back(pending);
		}
		_pending_cbs.clear();
		_cb_mutex.unlock();
	}

	static thread_local MyVector<VkFence> _fences;

	void LinearExecutor::waitOnCommandCompletion(bool global_wait, uint64_t timeout)
	{
		VkResult result = VK_SUCCESS;

		_cb_mutex.lock();
		if (global_wait)
		{
			result = vkDeviceWaitIdle(device());
		}
		else
		{
			_fences.clear();
			for (auto it = _previous_cbs.begin(), end = _previous_cbs.end(); it != end; ++it)
			{
				_fences.push_back(it->get()->signal_fence->handle());
			}
			if (_fences)
			{
				result = vkWaitForFences(device(), _fences.size32(), _fences.data(), VK_TRUE, timeout);
			}
		}
		VK_CHECK(result, "Failed to wait for all completion");

		for (auto it = _previous_cbs.begin(), end = _previous_cbs.end(); it != end; ++it)
		{
			VkResult result = it->get()->signal_fence->getStatus();
			//assert(result != VK_NOT_READY);
			it->get()->finish(result);
		}
		_previous_cbs.clear();
		_cb_mutex.unlock();
	}

	void LinearExecutor::waitOnSwapchainCompletion(bool global_wait, uint64_t timeout)
	{
		VkResult result = VK_SUCCESS;
		// It appears that after a call to vkDeviceWaitIdle, waiting on present signal fence ... waits forever. (with VkPresentFence) 
		// So we don't do it.
		_fences.clear();
		_swapchain_mutex.lock();
		for (auto it = _previous_swapchain_events.begin(), end = _previous_swapchain_events.end(); it != end; ++it)
		{
			_fences.push_back(it->get()->aquire_signal_fence->handle());
			//fences.push_back(it->get()->present_signal_fence->handle());
			//VkResult result = it->get()->signal_fence->getStatus();
			//assert(result != VK_NOT_READY);
		}
		if (_fences)
		{
			result = vkWaitForFences(device(), _fences.size32(), _fences.data(), VK_TRUE, timeout);
		}
		_previous_swapchain_events.clear();

		_swapchain_mutex.unlock();
	}

	void LinearExecutor::waitForAllCompletion(uint64_t timeout)
	{
		waitOnCommandCompletion(false, timeout);
		waitOnSwapchainCompletion(false, timeout);
	}
}