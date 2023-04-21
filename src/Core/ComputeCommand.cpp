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
		std::shared_ptr<Shader> shader = std::make_shared<Shader>(Shader::CI{
			.app = application(),
			.name = _shader_path.string(),
			.source_path = _shader_path,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.definitions = _definitions,
			});
		_program = std::make_shared<ComputeProgram>(ComputeProgram::CI{
			.app = application(),
			.name = shader->name(),
			.shader = shader,
		});
		_pipeline = std::make_shared<Pipeline>(Pipeline::ComputeCreateInfo{
			.app = application(),
			.name = _program->name(),
			.program = _program,
		});
		_sets = std::make_shared<DescriptorSetsManager>(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + ".sets",
			.bindings = ci.bindings,
			.program = _program,
		});
	}

	void ComputeCommand::init()
	{

	}

	void ComputeCommand::recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context)
	{
		InputSynchronizationHelper synch(context);
		recordBindings(cmd, context);

		_sets->instance()->recordInputSynchronization(synch);
		synch.record();

		const VkExtent3D workgroups = getWorkgroupsDispatchSize();
		vkCmdDispatch(cmd, workgroups.width, workgroups.height, workgroups.depth);

		synch.NotifyContext();

		context.keppAlive(_pipeline->instance());
		context.keppAlive(_sets->instance());
	}

	void ComputeCommand::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordCommandBuffer(*cmd, context);
	}

	bool ComputeCommand::updateResources()
	{
		bool res = false;
		
		res |= _pipeline->updateResources();
		

		res |= ShaderCommand::updateResources();

		return res;
	}
}