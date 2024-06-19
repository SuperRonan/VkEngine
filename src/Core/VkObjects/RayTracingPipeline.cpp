
#include <Core/VkObjects/RayTracingPipeline.hpp>

namespace vkl
{
	RayTracingPipelineInstance::RayTracingPipelineInstance(CreateInfo const& ci):
		PipelineInstance(PipelineInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.binding = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			.program = ci.program,
		}),
		_max_recursion_depth(ci.max_recursion_depth)
	{
		assert(application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline != VK_FALSE);
		RayTracingProgramInstance & prog = *program();
		MyVector<VkPipelineShaderStageCreateInfo> stages(prog.shaders().size());
		for (size_t i = 0; i < stages.size(); ++i)
		{
			stages[i] = prog.shaders()[i]->getPipelineShaderStageCreateInfo();
		}
		VkRayTracingPipelineCreateInfoKHR vk_ci{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0, // TODO, also flags2
			.stageCount = stages.size32(),
			.pStages = stages.data(),
			.groupCount = prog.shaderGroups().size32(),
			.pGroups = prog.shaderGroups().data(),
			.maxPipelineRayRecursionDepth = _max_recursion_depth,
			.pLibraryInfo = nullptr,
			.pLibraryInterface = nullptr,
			.layout = layout()->handle(),
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0,
		};
		application()->extFunctions()._vkCreateRayTracingPipelinesKHR(device(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &vk_ci, nullptr, &_handle);

		const uint32_t shader_group_handle_size = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupHandleSize;
		_shader_group_handles.resize(shader_group_handle_size * prog.shaderGroups().size());
		application()->extFunctions()._vkGetRayTracingShaderGroupHandlesKHR(device(), _handle, 0, prog.shaderGroups().size32(), _shader_group_handles.byte_size(), _shader_group_handles.data());
	}

	VkDeviceSize RayTracingPipelineInstance::getShaderGroupStackSize(uint32_t group_id, VkShaderGroupShaderKHR shader) const
	{
		return application()->extFunctions()._vkGetRayTracingShaderGroupStackSizeKHR(device(), _handle, group_id, shader);
	}


	RayTracingPipeline::RayTracingPipeline(CreateInfo const& ci):
		Pipeline(Pipeline::CI{
			.app = ci.app,
			.name = ci.name,
			.binding = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			.program = ci.program,
			.hold_instance = ci.hold_instance,
		}),
		_max_recursion_depth(ci.max_recursion_depth)
	{}

	void RayTracingPipeline::createInstanceIFP()
	{
		_inst = std::make_shared<RayTracingPipelineInstance>(RayTracingPipelineInstance::CI{
			.app = application(),
			.name = name(),
			.program = std::static_pointer_cast<RayTracingProgramInstance>(_program->instance()),
			.max_recursion_depth = _max_recursion_depth.value(),
		});
	}

	bool RayTracingPipeline::checkInstanceParamsReturnInvalid()
	{
		bool res = false;
		if (_inst)
		{
			RayTracingPipelineInstance * inst = static_cast<RayTracingPipelineInstance*>(_inst.get());
			if (inst->maxRecursionDepth() != _max_recursion_depth.value())
			{
				res = true;
			}
		}
		return res;
	}
}