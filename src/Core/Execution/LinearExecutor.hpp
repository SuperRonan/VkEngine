#pragma once

#include "Executor.hpp"
#include "ExecutionContext.hpp"
#include <Core/VkObjects/VkWindow.hpp>
#include <queue>
#include <Core/Commands/TransferCommand.hpp>
#include <Core/Commands/ImguiCommand.hpp>
#include "StagingPool.hpp"

namespace vkl
{
	class LinearExecutor : public Executor
	{
	protected:

		size_t _frame_index = size_t(-1);

		std::shared_ptr<VkWindow> _window = nullptr;

		VkWindow::AquireResult _aquired;

		std::shared_ptr<CommandBuffer> _command_buffer_to_submit = nullptr;

		StagingPool _staging_pool;

		std::shared_ptr<BlitImage> _blit_to_present = nullptr;
		std::shared_ptr<ImguiCommand> _render_gui = nullptr;

		ResourcesLists _internal_resources;

		MountingPoints * _mounting_points;

		ExecutionContext _context;

		ExecutionThread * _current_thread = nullptr;

		struct InBetween
		{
			std::shared_ptr<CommandBuffer> prev_cb = nullptr;
			std::shared_ptr<CommandBuffer> next_cb = nullptr;
			std::vector<std::shared_ptr<Fence>> fences = {};
			std::shared_ptr<Semaphore> semaphore = nullptr;
			std::vector<std::shared_ptr<VkObject>> dependecies = {};
		};

		void stackInBetween();

		std::queue<InBetween> _previous_in_betweens;
		InBetween _in_between;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<VkWindow> window = nullptr;
			MountingPoints * mounting_points = nullptr;
			bool use_ImGui = false;
			bool use_debug_renderer = true;
			bool use_ray_tracing = false;
		};

		using CI = CreateInfo;

		LinearExecutor(CreateInfo const& ci);

		virtual ~LinearExecutor() override;

		virtual void init() override final;

		void updateResources(UpdateContext & context);

		void beginFrame();

		ExecutionThread * beginCommandBuffer(bool bind_common_set = true);

		void bindSet(uint32_t s, std::shared_ptr<DescriptorSetAndPool> const& set, bool bind_graphics = true, bool bind_compute = true, bool bind_rt = true);

		void renderDebugIFP();

		void preparePresentation(std::shared_ptr<ImageView> img_to_present, bool render_ImGui = true);

		void endCommandBufferAndSubmit(ExecutionThread * exec_thread);

		void present();

		virtual void execute(Command & cmd);

		virtual void execute(std::shared_ptr<Command> cmd);

		virtual void execute(Executable const& executable);

		void submit();

		virtual void waitForAllCompletion(uint64_t timeout = UINT64_MAX) override final;

		void waitForCurrentCompletion(uint64_t timeout = UINT64_MAX);

		std::shared_ptr<CommandBuffer> getCommandBuffer()
		{
			return _command_buffer_to_submit;
		}
	};
}