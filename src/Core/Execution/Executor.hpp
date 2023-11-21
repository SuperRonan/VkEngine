#pragma once

#include <Core/Commands/Command.hpp>
#include <string>

#include <Core/Execution/DefinitionMap.hpp>
#include <Core/Commands/ShaderCommand.hpp>
#include <Core/Execution/ResourcesLists.hpp>

namespace vkl
{
	using namespace std::chrono_literals;

	class DebugRenderer;

	class ExecutionRecorder : public VkObject
	{
	protected:
		
	public:

		ExecutionRecorder(VkApplication * app, std::string const& name);


		virtual void record(Command& cmd) = 0;

		virtual void record(std::shared_ptr<Command> cmd) = 0;

		virtual void record(Executable const& executable) = 0;

		
		void execute(Command& cmd)
		{
			record(cmd);
		}

		void execute(std::shared_ptr<Command> cmd)
		{
			record(cmd);
		}

		void execute(Executable const& executable)
		{
			record(executable);
		}

		
		void operator()(std::shared_ptr<Command> cmd)
		{
			execute(cmd);
		}

		void operator()(Command& cmd)
		{
			execute(cmd);
		}

		void operator()(Executable const& executable)
		{
			execute(executable);
		}

		
		virtual void bindSet(uint32_t s, std::shared_ptr<DescriptorSetAndPool> const& set, bool bind_graphics = true, bool bind_compute = true, bool bind_rt = true) = 0;
	};

	class Executor : public VkObject
	{
	protected:

		DefinitionsMap _common_definitions;


		bool _use_debug_renderer = true;
		bool _use_rt_pipeline = false;
		
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
			bool use_debug_renderer = true;
			bool use_ray_tracing_pipeline = false;
		};
		using CI = CreateInfo;

		Executor(CreateInfo const& ci):
			VkObject(ci.app, ci.name),
			_use_debug_renderer(ci.use_debug_renderer),
			_use_rt_pipeline(ci.use_ray_tracing_pipeline)
		{}

		virtual void init() = 0;

		virtual void waitForAllCompletion(uint64_t timeout = UINT64_MAX) = 0;

		DefinitionsMap& getCommonDefinitions()
		{
			return _common_definitions;
		}

		const DefinitionsMap& getCommonDefinitions()const
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