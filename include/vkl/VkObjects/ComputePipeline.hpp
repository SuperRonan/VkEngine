#pragma once

#include <vkl/VkObjects/Pipeline.hpp>
#include <vkl/VkObjects/ComputeProgram.hpp>

namespace vkl
{
	class ComputePipelineInstance : public PipelineInstance
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ComputeProgramInstance> program = nullptr;
		};
		using CI = CreateInfo;

	protected:

	public:

		ComputePipelineInstance(CreateInfo const& ci);

		virtual ~ComputePipelineInstance() override = default;

		ComputeProgramInstance* program()const
		{
			return static_cast<ComputeProgramInstance*>(_program.get());
		}
	};

	class ComputePipeline : public Pipeline
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ComputeProgram> program = nullptr;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

	protected:

		virtual void createInstanceIFP() override;

		virtual bool checkInstanceParamsReturnInvalid() override;

	public:

		ComputePipeline(CreateInfo const& ci);

		virtual ~ComputePipeline() override = default;

		ComputePipelineInstance* instance() const
		{
			return static_cast<ComputePipelineInstance*>(_instance.get());
		}

		std::shared_ptr<ComputePipelineInstance> const& instancePtr() const
		{
			return std::reinterpret_pointer_downcast<ComputePipelineInstance>(_instance);
		}

		ComputeProgram* program() const
		{
			return static_cast<ComputeProgram*>(_program.get());
		}

		std::shared_ptr<ComputeProgram> const& programPtr() const
		{
			return std::reinterpret_pointer_downcast<ComputeProgram>(_program);
		}
	};
}