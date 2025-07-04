#pragma once

#include <vkl/Commands/Command.hpp>
#include <string>

#include <vkl/Commands/ShaderCommand.hpp>
#include <vkl/Execution/DefinitionMap.hpp>
#include <vkl/Execution/ResourcesLists.hpp>
#include <vkl/Execution/FramePerformanceCounters.hpp>

#include <vkl/Execution/RenderPassBeginInfo.hpp>

namespace vkl
{
	using namespace std::chrono_literals;

	class DebugRenderer;

	struct BindSetInfo
	{
		uint32_t index = 0;
		std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		bool bind_graphics = true;
		bool bind_compute = true;
		bool bind_rt = true;
	};

	class ExecutionRecorder : public VkObject
	{
	protected:

		FramePerfCounters * _frame_perf_counters = nullptr;
		
	public:

		ExecutionRecorder(VkApplication * app, std::string const& name);


		virtual void record(Command& cmd) = 0;

		virtual void record(std::shared_ptr<Command> cmd) = 0;

		virtual void record(Executable const& exec) = 0;

		
		void execute(Command& cmd)
		{
			record(cmd);
		}

		void execute(std::shared_ptr<Command> cmd)
		{
			record(cmd);
		}

		void execute(Executable const& node)
		{
			record(node);
		}

		
		void operator()(std::shared_ptr<Command> cmd)
		{
			execute(cmd);
		}

		void operator()(Command& cmd)
		{
			execute(cmd);
		}

		void operator()(Executable const& node)
		{
			execute(node);
		}

		
		
		virtual void bindSet(BindSetInfo const& info) = 0;

		virtual void beginRenderPass(RenderPassBeginInfo const& info, RenderPassBeginInfo::Flags flags = RenderPassBeginInfo::Flags::None) = 0;

		virtual void nextSubPass(RenderPassBeginInfo::Flags flags = RenderPassBeginInfo::Flags::None) = 0;

		struct SubpassInfo
		{
			uint32_t index = 0;
			RenderPassBeginInfo::Flags flags = RenderPassBeginInfo::Flags::None;
		};

		virtual bool getCurrentRenderingStatus(const RenderPassBeginInfo ** info = nullptr, SubpassInfo * subpass_info = nullptr) const = 0;

		virtual void endRenderPass() = 0;

		using vec4 = Vector4f;

		virtual void pushDebugLabel(std::string_view const& label, vec4 const& color, bool timestamp = false) = 0;

		virtual void popDebugLabel() = 0;

		virtual void insertDebugLabel(std::string_view const& label, vec4 const& color) = 0;

		void pushDebugLabel(std::string_view const& label, bool timestamp = false);

		FramePerfCounters* framePerfCounters()const
		{
			return _frame_perf_counters;
		}

		virtual void setFramePerfCounters(FramePerfCounters* fpc)
		{
			_frame_perf_counters = fpc;
		}
	};

	class Executor : public VkObject
	{
	protected:

		bool _use_debug_renderer = true;
		bool _use_rt_pipeline = false;
		
		DefinitionsMap * _common_definitions = nullptr;
		
		std::shared_ptr<DebugRenderer> _debug_renderer = nullptr;

		std::shared_ptr<DescriptorSetLayout> _common_set_layout;
		std::shared_ptr<DescriptorSetAndPool> _common_descriptor_set;

		void buildCommonSetLayout();

		void createDebugRenderer();

		void createCommonSet();

	public:

		struct CreateInfo 
		{
			VkApplication * app = nullptr;
			std::string name = {};
			DefinitionsMap * common_definitions = nullptr;
			bool use_debug_renderer = true;
			bool use_ray_tracing_pipeline = false;
		};
		using CI = CreateInfo;

		Executor(CreateInfo const& ci):
			VkObject(ci.app, ci.name),
			_use_debug_renderer(ci.use_debug_renderer),
			_use_rt_pipeline(ci.use_ray_tracing_pipeline),
			_common_definitions(ci.common_definitions)
		{}

		virtual void init() = 0;

		virtual void waitForAllCompletion(uint64_t timeout = UINT64_MAX) = 0;

		DefinitionsMap * getCommonDefinitions()const
		{
			return _common_definitions;
		}

		std::shared_ptr<DescriptorSetLayout> const& getCommonSetLayout()const
		{
			return _common_set_layout;
		}

		std::shared_ptr<DebugRenderer> const& getDebugRenderer() const
		{
			return _debug_renderer;
		}
	};
}