#pragma once

#include "Executor.hpp"
#include "ExecutionContext.hpp"

#include <queue>

#include <vkl/Commands/TransferCommand.hpp>
#include <vkl/Commands/GraphicsTransferCommands.hpp>
#include <vkl/Commands/ImguiCommand.hpp>

#include <vkl/VkObjects/VkWindow.hpp>
#include <vkl/VkObjects/Queue.hpp>

namespace vkl
{
	
	// "Events"
	struct CommandBufferSubmission;
	struct SwapchainEvent;

	class ExecutionThread : public ExecutionRecorder
	{
	public:

	protected:


		RecordContext _record_context;
		ExecutionContext* _context;

		uint32_t _current_render_pass_index = uint32_t(-1);
		uint32_t _current_subpass_index = 0;
		bool _deferred_record = false;
		bool _render_pass_synch_subpass = false;

		struct CommandEvent
		{
			enum class Type : uint32_t
			{
				ExecNode,
				BindSet,
				BeginRenderPass,
				NextSubPass,
				EndRenderPass,
				PushDebugLabel,
				PopDebugLabel,
				InsertDebugLabel,
				MAX_ENUM = ~0u,
			};

			Type type = Type::MAX_ENUM;
			union
			{
				uint32_t index = 0;
				RenderPassBeginInfo::Flags flags;
			};
		};

		struct ExecNodeEvent
		{
			std::shared_ptr<ExecutionNode> node;
		};

		struct BindSetEvent
		{
			BindSetInfo info = {};
		};

		// Note on RenderPass's contents:
		// It could be deduced from the commands recorded during each subpass scope
		struct BeginRenderPassEvent
		{
			RenderPassBeginInfo info = {};
			RenderPassBeginInfo::Flags flags = RenderPassBeginInfo::Flags::None;
		};

		struct DebugLabelEvent
		{
			uint32_t string_index = 0;
			uint32_t string_len = 0;
			vec4 color = {};
			bool timestamp = false;
		};

		that::ExS<ExecNodeEvent> _nodes = {};
		that::ExS<BindSetEvent> _sets = {};
		that::ExS<VkClearValue> _clear_values = {};
		that::ExS<BeginRenderPassEvent> _begin_render_passes = {};
		that::ExS<DebugLabelEvent> _debug_labels = {};
		that::ExSS _strings = {};
		
		MyVector<CommandEvent> _commands;

		ResourceUsageList _render_pass_resources;
		SynchronizationHelper _synch;

		void clearDeferedLists();

		void executeNode(std::shared_ptr<ExecutionNode> const &node);

		void recordEventNotRenderPass(uint32_t index, bool synch);

		bool useDeferredRecord() const;

		void releaseNodes();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			ExecutionContext* context;
			bool deferred_record = false;
		};
		using CI = CreateInfo;

		ExecutionThread(CreateInfo const& ci);

		virtual void record(Command& cmd) override;

		virtual void record(std::shared_ptr<Command> cmd) override;

		virtual void record(Executable const& executable) override;

		virtual void bindSet(BindSetInfo const& info) override;

		virtual void beginRenderPass(RenderPassBeginInfo const& info, RenderPassBeginInfo::Flags flags = RenderPassBeginInfo::Flags::None) override;

		virtual bool getCurrentRenderingStatus(const RenderPassBeginInfo ** info = nullptr, SubpassInfo* subpass_info = nullptr) const override;

		virtual void nextSubPass(RenderPassBeginInfo::Flags flags = RenderPassBeginInfo::Flags::None) override;

		virtual void endRenderPass() override;

		using vec4 = glm::vec4;

		virtual void pushDebugLabel(std::string_view const& label, vec4 const& color, bool timestamp = false) override;

		// "Once you overload a function from Base class in Derived class all functions with the same name in the Base class get hidden in Derived class."
		void pushDebugLabel(std::string_view const& label, bool timestamp = false)
		{
			ExecutionRecorder::pushDebugLabel(label, timestamp);
		}

		virtual void popDebugLabel() override;

		virtual void insertDebugLabel(std::string_view const& label, vec4 const& color) override;

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

		void reset();

		void recordCommands();
	};

	struct FramePerfReport;
	
	class LinearExecutor : public Executor
	{
	protected:

		size_t _frame_index = size_t(-1);
		size_t _cb_count = 0;
		std::TickTock_hrc _frame_tt;

		size_t _fifo_fence_to_wait_index = 0;
		MyVector<std::shared_ptr<Fence>> _fifo_aquire_fences;
		std::shared_ptr<VkWindow> _window = nullptr;

		std::shared_ptr<Queue> _main_queue = nullptr;
		std::shared_ptr<Queue> _present_queue = nullptr;

		std::shared_ptr<BlitImage> _blit_to_present = nullptr;
		std::shared_ptr<ImguiCommand> _render_gui = nullptr;

		std::shared_ptr<QueryPool> _timestamp_query_pool = nullptr;

		ResourcesLists _internal_resources;

		ExecutionContext _context;

		std::unique_ptr<ExecutionThread> _execution_thread;

		ExecutionThread* _current_thread = nullptr;

		std::deque<std::shared_ptr<CommandBufferSubmission>> _previous_cbs = {};
		std::deque<std::shared_ptr<SwapchainEvent>> _previous_swapchain_events = {};

		MyVector<std::shared_ptr<CommandBufferSubmission>> _pending_cbs = {};
		
		std::shared_ptr<CommandBufferSubmission> _latest_synch_cb = nullptr;
		std::shared_ptr<SwapchainEvent> _latest_swapchain_event = nullptr;

		std::shared_ptr<AsynchTask> _previous_recycle_task;

		MyVector<std::shared_ptr<CommandBuffer>> _trash_cbs;
		void recyclePreviousEvents();

		std::mutex _cb_mutex;
		std::mutex _trash_mutex;
		std::mutex _swapchain_mutex;

		bool useSpecificPresentSignalFence() const;

		MyVector<std::shared_ptr<FramePerfReport>> _frame_perf_report_pool = {};
		uint32_t _frame_timestamp_query_count = 0;
		uint32_t _timestamp_query_pool_capacity = 128;
		std::shared_ptr<FramePerfReport> _current_frame_report = nullptr;
		std::mutex _frame_perf_report_pool_mutex;
		std::shared_ptr<FramePerfReport> _pending_frame_report = nullptr;

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

		void beginFrame(bool capture_report);

		void endFrame();

		void aquireSwapchainImage();

		//ExecutionThread* beginTransferCommandBuffer();

		void performSynchTransfers(UpdateContext & update_context, bool consume_asynch = true, TransferBudget budget = TransferBudget{.bytes = size_t(16'000'000), .instances = size_t(128)});

		void performAsynchMipsCompute(MipMapComputeQueue & mips_queue, TransferBudget budget = TransferBudget{ .bytes = size_t(16'000'000), .instances = size_t(128) });

		ExecutionThread* beginCommandBuffer(bool bind_common_set = true);

		void bindSet(BindSetInfo const& info);

		void renderDebugIFP();

		void preparePresentation(std::shared_ptr<ImageView> img_to_present, bool render_ImGui = true);

		void endCommandBuffer(ExecutionThread* exec_thread, bool submit = false);

		void submit();

		void present();

		virtual void execute(Command & cmd);

		virtual void execute(std::shared_ptr<Command> cmd);

		virtual void execute(Executable const& executable);

		void waitOnCommandCompletion(bool global_wait = false, uint64_t timeout = UINT64_MAX);

		void waitOnSwapchainCompletion(bool global_wait = false, uint64_t timeout = UINT64_MAX);

		virtual void waitForAllCompletion(uint64_t timeout = UINT64_MAX) override final;

		std::shared_ptr<FramePerfReport> const& getPendingFrameReport()
		{
			return _pending_frame_report;
		}
	};
}