#include "ComputeCommand.hpp"

namespace vkl
{
	ComputeCommand::ComputeCommand(CreateInfo const& ci) :
		ShaderCommand(ci.app, ci.name, ci.bindings),
		_shader_path(ci.shader_path),
		_definitions(ci.definitions),
		_dispatch_size(ci.dispatch_size),
		_dispatch_threads(ci.dispatch_threads)
	{
		
	}

	void ComputeCommand::init()
	{

	}

	void ComputeCommand::recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context)
	{
		InputSynchronizationHelper synch(context);
		recordBindings(cmd, context);

		_sets.recordInputSynchronization(synch);
		synch.record();

		const VkExtent3D workgroups = getWorkgroupsDispatchSize();
		vkCmdDispatch(cmd, workgroups.width, workgroups.height, workgroups.depth);

		synch.NotifyContext();
	}

	void ComputeCommand::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordCommandBuffer(*cmd, context);
	}

	bool ComputeCommand::updateResources()
	{
		bool res = false;
		if (!_pipeline)
		{
			Shader shader(application(), _shader_path, VK_SHADER_STAGE_COMPUTE_BIT, _definitions);
			_program = std::make_shared<ComputeProgram>(std::move(shader));
			_pipeline = std::make_shared<Pipeline>(_program);
			
			_sets.setProgram(_pipeline->program());
			_sets.invalidateDescriptorSets();
			_sets.resolveBindings();

			res = true;

		}

		res |= ShaderCommand::updateResources();

		return res;
	}
}