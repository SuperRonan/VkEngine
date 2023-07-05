#include "ShaderCommand.hpp"

namespace vkl
{
	void ShaderCommand::recordPushConstant(CommandBuffer& cmd, ExecutionContext& context, PushConstant const& pc)
	{
		if (pc.hasValue())
		{
			VkShaderStageFlags pc_stages = 0;
			for (const auto& pc_range : _pipeline->program()->instance()->pushConstantRanges())
			{
				pc_stages |= pc_range.stageFlags;
			}
			vkCmdPushConstants(cmd, *_pipeline->program()->instance()->pipelineLayout(), pc_stages, 0, (uint32_t)pc.size(), pc.data());
		}
	}

	void ShaderCommand::recordBindings(CommandBuffer& cmd, ExecutionContext& context)
	{
		vkCmdBindPipeline(cmd, _pipeline->instance()->binding(), *_pipeline->instance());

		_sets->instance()->recordBindings(cmd, _pipeline->instance()->binding());
	}

	void DescriptorSetsInstance::recordInputSynchronization(SynchronizationHelper& synch)
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			if (_bindings[i].isResolved() && _bindings[i].updated())
			{
				synch.addSynch(_bindings[i].resource());
			}
		}
	}


	bool ShaderCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= _pipeline->updateResources(ctx);

		res |= _sets->updateResources(ctx);

		_sets->instance()->writeDescriptorSets();

		return res;
	}
}