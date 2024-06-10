
#include <Core/VkObjects/ComputePipeline.hpp>

namespace vkl
{
	
	ComputePipelineInstance::ComputePipelineInstance(CreateInfo const& ci) :
		PipelineInstance(PipelineInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.binding = VK_PIPELINE_BIND_POINT_COMPUTE,
			.program = ci.program,
		})
	{
		const VkComputePipelineCreateInfo vk_ci{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = program()->shader()->getPipelineShaderStageCreateInfo(),
			.layout = *_program->pipelineLayout(),
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0,
		};
		VK_CHECK(vkCreateComputePipelines(_app->device(), nullptr, 1, &vk_ci, nullptr, &_handle), "Failed to create a compute pipeline.");
	}

		
	ComputePipeline::ComputePipeline(CreateInfo const& ci) :
		Pipeline(Pipeline::CI{
			.app = ci.app,
			.name = ci.name,
			.binding = VK_PIPELINE_BIND_POINT_COMPUTE,
			.program = ci.program,
			.hold_instance = ci.hold_instance,
		})
	{

	}

	bool ComputePipeline::checkInstanceParamsReturnInvalid()
	{
		return false;
	}

	void ComputePipeline::createInstanceIFP()
	{
		_inst = std::make_shared<ComputePipelineInstance>(ComputePipelineInstance::CI{
			.app = application(),
			.name = name(),
			.program = std::static_pointer_cast<ComputeProgramInstance>(_program->instance()),
		});
	}
}