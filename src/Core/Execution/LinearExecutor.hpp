#pragma once

#include "Executor.hpp"
#include "ExecutionContext.hpp"
#include <Core/VkObjects/VkWindow.hpp>
#include <queue>
#include <Core/Commands/TransferCommand.hpp>
#include <Core/Commands/GraphicsTransferCommands.hpp>
#include <Core/Commands/ImguiCommand.hpp>

#include <Core/Execution/BufferPool.hpp>

namespace vkl
{
	class ExecutionThread : public ExecutionRecorder
	{
	protected:

		RecordContext _record_context;
		ExecutionContext* _context;

		void executeNode(std::shared_ptr<ExecutionNode> const &node);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			ExecutionContext* context;
		};
		using CI = CreateInfo;

		ExecutionThread(CreateInfo const& ci);

		virtual void record(Command& cmd) override;

		virtual void record(std::shared_ptr<Command> cmd) override;

		virtual void record(Executable const& executable) override;

		virtual void bindSet(uint32_t s, std::shared_ptr<DescriptorSetAndPool> const& set, bool bind_graphics = true, bool bind_compute = true, bool bind_rt = true) override;

		using vec4 = glm::vec4;

		virtual void pushDebugLabel(std::string const& label, vec4 const& color) override;

		virtual void popDebugLabel() override;

		virtual void insertDebugLabel(std::string const& label, vec4 const& color) override;

		ExecutionContext* context()
		{
			return _context;
		}

		virtual void setFramePerfCounters(FramePerfCounters* fpc) override
		{
			ExecutionRecorder::setFramePerfCounters(fpc);
			if (_context)
			{
				_context->setFramePerfCounters(fpc);
			}
		}
	};
	
	class LinearExecutor : public Executor
	{
	protected:

		size_t _frame_index = size_t(-1);
		size_t _cb_count = 0;

		std::shared_ptr<VkWindow> _window = nullptr;

		BufferPool _staging_pool;

		std::shared_ptr<BlitImage> _blit_to_present = nullptr;
		std::shared_ptr<ImguiCommand> _render_gui = nullptr;

		ResourcesLists _internal_resources;

		ExecutionContext _context;

		ExecutionThread* _current_thread = nullptr;

		// Event is not a good name imo (means something else in vulkan)
		struct Event : public VkObject
		{	
			enum class Type {
				CommandBuffer,
				SwapchainAquire,
				Present,
				MAX_ENUM,
			};
			Type type = Type::MAX_ENUM;
			
			std::shared_ptr<CommandBuffer> cb = nullptr;
			VkQueue queue = VK_NULL_HANDLE;
			
			std::shared_ptr<SwapchainInstance> swapchain = nullptr;
			uint32_t aquired_id = -1;

			std::vector<std::shared_ptr<Semaphore>> wait_semaphores = {};
			std::shared_ptr<Semaphore> signal_semaphore = nullptr;

			//std::vector<std::shared_ptr<Fence>> wait_fences = {};
			std::shared_ptr<Fence> signal_fence = nullptr;

			std::vector<std::shared_ptr<VkObject>> dependecies = {};

			uint32_t finish_counter = 0;			

			Event(VkApplication* app, std::string const& name, Type type, bool create_synch):
				VkObject(app, name),
				type(type)
			{
				if (create_synch)
				{
					signal_semaphore = std::make_shared<Semaphore>(application(), this->name() + ".SignalSemaphore");
					signal_fence = std::make_shared<Fence>(application(), this->name() + ".SignalFence");
				}
			}

			virtual ~Event() override
			{
				int _ = 0;
			}
		};

		std::queue<std::shared_ptr<Event>> _previous_events = {};

		std::vector<std::shared_ptr<Event>> _pending_cbs = {};
		
		std::shared_ptr<Event> _latest_synch_cb = nullptr;
		std::shared_ptr<Event> _latest_aquire_event = nullptr;
		std::shared_ptr<Event> _latest_present_event = nullptr;

		void recyclePreviousEvents();

		std::mutex _mutex;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<VkWindow> window = nullptr;
			MountingPoints * mounting_points = nullptr;
			DefinitionsMap * common_definitions = nullptr;
			bool use_ImGui = false;
			bool use_debug_renderer = true;
			bool use_ray_tracing = false;
		};

		using CI = CreateInfo;

		LinearExecutor(CreateInfo const& ci);

		virtual ~LinearExecutor() override;

		virtual void init() override final;

		void updateResources(UpdateContext & context);

		void AquireSwapchainImage();

		//ExecutionThread* beginTransferCommandBuffer();

		void performSynchTransfers(UpdateContext & update_context, bool consume_asynch = true, TransferBudget budget = TransferBudget{.bytes = size_t(16'000'000), .instances = size_t(128)});

		void performAsynchMipsCompute(MipMapComputeQueue & mips_queue, TransferBudget budget = TransferBudget{ .bytes = size_t(16'000'000), .instances = size_t(128) });

		ExecutionThread* beginCommandBuffer(bool bind_common_set = true);

		void bindSet(uint32_t s, std::shared_ptr<DescriptorSetAndPool> const& set, bool bind_graphics = true, bool bind_compute = true, bool bind_rt = true);

		void renderDebugIFP();

		void preparePresentation(std::shared_ptr<ImageView> img_to_present, bool render_ImGui = true);

		void endCommandBuffer(ExecutionThread* exec_thread, bool submit = false);

		void submit();

		void present();

		virtual void execute(Command & cmd);

		virtual void execute(std::shared_ptr<Command> cmd);

		virtual void execute(Executable const& executable);

		virtual void waitForAllCompletion(uint64_t timeout = UINT64_MAX) override final;
	};
}