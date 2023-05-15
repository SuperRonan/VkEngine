#pragma once

#include "Executor.hpp"
#include "ExecutionContext.hpp"
#include "VkWindow.hpp"
#include <queue>
#include "TransferCommand.hpp"
#include "ImguiCommand.hpp"

namespace vkl
{
	class LinearExecutor : public Executor
	{
	protected:

		size_t _frame_index = size_t(-1);

		std::shared_ptr<VkWindow> _window = nullptr;

		VkWindow::AquireResult _aquired;

		std::shared_ptr<CommandBuffer> _command_buffer_to_submit = nullptr;

		ResourceStateTracker _resources_state = {};

		ExecutionContext _context;

		std::shared_ptr<BlitImage> _blit_to_present = nullptr;
		std::shared_ptr<ImguiCommand> _render_gui = nullptr;

		std::vector<std::shared_ptr<Command>> _commands = {};

		std::vector<std::shared_ptr<ImageView>> _registered_images = {};
		std::vector<std::shared_ptr<Buffer>> _registered_buffers = {};
		std::vector<std::shared_ptr<Sampler>> _registered_samplers = {};

		void preprocessCommands();

		void preprocessResources();

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


		std::chrono::milliseconds _shader_check_period = 1s;
		std::chrono::time_point<std::chrono::system_clock> _shader_check_time = std::chrono::system_clock::now();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<VkWindow> window = nullptr;
			bool use_ImGui = false;
		};

		using CI = CreateInfo;

		LinearExecutor(CreateInfo const& ci);

		virtual ~LinearExecutor() override;

		virtual void declare(std::shared_ptr<Command> cmd) override final;

		virtual void declare(std::shared_ptr<ImageView> view) override final;

		virtual void declare(std::shared_ptr<Buffer> buffer) override final;

		virtual void declare(std::shared_ptr<Sampler> sampler) override final;

		virtual void init() override final;

		virtual void updateResources() override final;

		void beginFrame();

		void beginCommandBuffer();

		void preparePresentation(std::shared_ptr<ImageView> img_to_present, bool render_ImGui = true);

		void endCommandBufferAndSubmit();

		void present();

		virtual void execute(std::shared_ptr<Command> cmd) override final;

		virtual void execute(Executable const& executable) override final;

		void submit();

		virtual void waitForAllCompletion(uint64_t timeout = UINT64_MAX) override final;

		void waitForCurrentCompletion(uint64_t timeout = UINT64_MAX);

		std::shared_ptr<CommandBuffer> getCommandBuffer()
		{
			return _command_buffer_to_submit;
		}
	};
}