#include "ComputeCommand.hpp"

namespace vkl
{
	ComputeCommand::ComputeCommand(CreateInfo const& ci) :
		ShaderCommand(ci.app, ci.name, ci.bindings),
		_program(new ComputeProgram(Shader(ci.app, ci.shader_path, VK_SHADER_STAGE_COMPUTE_BIT, ci.definitions))),
		_dispatch_size(ci.dispatch_size),
		_dispatch_threads(ci.dispatch_threads)
	{
		_pipeline = std::make_shared<Pipeline>(_app, _program);
	}

	void ComputeCommand::init()
	{
		resolveBindings();
		declareDescriptorSetsResources();
		writeDescriptorSets();
	}

	void ComputeCommand::recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context)
	{
		recordInputSynchronization(cmd, context);
		recordBindings(cmd, context);

		const VkExtent3D workgroups = getWorkgroupsDispatchSize();
		vkCmdDispatch(cmd, workgroups.width, workgroups.height, workgroups.depth);
	}

	void ComputeCommand::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCurrentCommandBuffer();
		recordCommandBuffer(*cmd, context);

		declareResourcesEndState(context);
	}

	
}