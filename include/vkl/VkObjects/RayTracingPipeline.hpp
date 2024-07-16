#pragma once

#include <vkl/VkObjects/Pipeline.hpp>
#include <vkl/VkObjects/RayTracingProgram.hpp>

namespace vkl
{
	class RayTracingPipelineInstance : public PipelineInstance
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<RayTracingProgramInstance> program = nullptr;
			uint32_t max_recursion_depth = 1;
		};
		using CI = CreateInfo;

	protected:

		uint32_t _max_recursion_depth = 1;

		MyVector<uint8_t> _shader_group_handles;

	public:

		RayTracingPipelineInstance(CreateInfo const& ci);

		virtual ~RayTracingPipelineInstance() override = default;

		RayTracingProgramInstance* program()const
		{
			return static_cast<RayTracingProgramInstance*>(_program.get());
		}

		uint32_t maxRecursionDepth()const
		{
			return _max_recursion_depth;
		}

		MyVector<uint8_t> const& shaderGroupHandles()const
		{
			return _shader_group_handles;
		}

		VkDeviceSize getShaderGroupStackSize(uint32_t group_id, VkShaderGroupShaderKHR shader) const;
	};

	class RayTracingPipeline : public Pipeline
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<RayTracingProgram> program = nullptr;
			Dyn<uint32_t> max_recursion_depth = {};
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

	protected:

		Dyn<uint32_t> _max_recursion_depth = {};

		virtual void createInstanceIFP() override;

		virtual bool checkInstanceParamsReturnInvalid() override;

	public:

		RayTracingPipeline(CreateInfo const& ci);

		virtual ~RayTracingPipeline() override = default;


		RayTracingProgram* program()const
		{
			return static_cast<RayTracingProgram*>(_program.get());
		}
	};
}