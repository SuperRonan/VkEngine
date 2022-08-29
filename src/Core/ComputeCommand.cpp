#include "ComputeCommand.hpp"

namespace vkl
{
	void ComputeCommand::recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context)
	{
		recordInputSynchronization(cmd, context);
		recordBindings(cmd, context);

		const VkExtent3D workgroups = _dispatch_threads ? VkExtent3D{
			.width = std::moduloCeil(_dispatch_size.width, _program->localSize().width),
			.height = std::moduloCeil(_dispatch_size.height, _program->localSize().height),
			.depth = std::moduloCeil(_dispatch_size.depth, _program->localSize().depth),
		} : _dispatch_size;
		vkCmdDispatch(cmd, workgroups.width, workgroups.height, workgroups.depth);
	}

	void ComputeCommand::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCurrentCommandBuffer();
		recordCommandBuffer(*cmd, context);
	}
}