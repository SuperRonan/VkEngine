#pragma once

#include <vkl/App/VkApplication.hpp>
#include "Program.hpp"

namespace vkl
{
	class PipelineInstance : public AbstractInstance
	{
	protected:

		VkPipeline _handle = VK_NULL_HANDLE;
		VkPipelineBindPoint _binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		std::shared_ptr<ProgramInstance> _program;

		void setVkNameIFP();

	public:


		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			VkPipelineBindPoint binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
			std::shared_ptr<ProgramInstance> program = nullptr;
		};
		using CI = CreateInfo;

		PipelineInstance(CreateInfo const& ci);

		virtual ~PipelineInstance() override;

		constexpr VkPipeline pipeline()const noexcept
		{
			return _handle;
		}

		constexpr auto handle()const noexcept
		{
			return pipeline();
		}

		constexpr VkPipelineBindPoint binding()const noexcept
		{
			return _binding;
		}

		constexpr operator VkPipeline()const
		{
			return _handle;
		}

		constexpr const std::shared_ptr<ProgramInstance>& getProgram()const
		{
			return _program;
		}

		ProgramInstance* program()const
		{
			return _program.get();
		}

		std::shared_ptr<PipelineLayoutInstance> const& layout()const
		{
			return _program->pipelineLayout();
		}
	};


	class Pipeline : public InstanceHolder<PipelineInstance>
	{
	public:

		using ParentType = InstanceHolder<PipelineInstance>;
		
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			VkPipelineBindPoint binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
			std::shared_ptr<Program> program = nullptr;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

	protected:


		VkPipelineBindPoint _binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		
		bool _latest_update_result = false;
		size_t _latest_update_tick = 0;
		
		std::shared_ptr<Program> _program = nullptr;


		mutable std::shared_ptr<AsynchTask> _create_instance_task = nullptr;

		void launchInstanceCreationTask();

		virtual void createInstanceIFP() = 0;

		virtual void destroyInstanceIFN() override;

		virtual bool checkInstanceParamsReturnInvalid() = 0;

	public:

		Pipeline(CreateInfo const& ci);

		virtual ~Pipeline() override;

		constexpr VkPipelineBindPoint binding()const noexcept
		{
			return _binding;
		}

		constexpr const std::shared_ptr<Program>& getProgram()const
		{
			return _program;
		}

		// Will be overriden
		Program * program()const
		{
			return _program.get();
		}

		bool updateResources(UpdateContext & ctx);

		bool instanceIsPending() const
		{
			return !!_create_instance_task;
		}

		bool hasInstanceOrIsPending() const
		{
			return _inst || instanceIsPending();
		}

		std::shared_ptr<AsynchTask> const& getInstanceCreationTask()const
		{
			return _create_instance_task;
		}

		void waitForInstanceCreationIFN();

		std::shared_ptr<PipelineInstance> getInstanceWaitIFN();
	};

}