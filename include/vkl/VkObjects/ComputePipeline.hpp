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

		ComputeProgram* program()const
		{
			return static_cast<ComputeProgram*>(_program.get());
		}
	};
}