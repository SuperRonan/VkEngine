#include "ComputeCommand.hpp"

namespace vkl
{
	ComputeCommand::ComputeCommand(CreateInfo const& ci) :
		ShaderCommand(ShaderCommand::CreateInfo{
			.app = ci.app, 
			.name = ci.name, 
			.sets_layouts = ci.sets_layouts,
		}),
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
			.sets_layouts = _provided_sets_layouts,
			.shader = shader,
		});

		_pipeline = std::make_shared<Pipeline>(Pipeline::ComputeCreateInfo{
			.app = application(),
			.name = _program->name(),
			.program = _program,
		});

		_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CI{
			.app = application(),
			.name = name() + ".shader_set",
			.program = _program,
			.target_set = application()->descriptorBindingGlobalOptions().shader_set,
			.bindings = ci.bindings,
		});
	}

	void ComputeCommand::init()
	{

	}

	void ComputeCommand::recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context, DispatchInfo const& di)
	{
		SynchronizationHelper synch(context);
		recordBindings(cmd, context);
		recordPushConstant(cmd, context, di.push_constant);

		recordBoundResourcesSynchronization(context.computeBoundSets(), synch, application()->descriptorBindingGlobalOptions().shader_set + 1);

		synch.record();

		const VkExtent3D workgroups = _dispatch_threads ? getWorkgroupsDispatchSize(di.extent) : di.extent;
		vkCmdDispatch(cmd, workgroups.width, workgroups.height, workgroups.depth);
	}

	void ComputeCommand::execute(ExecutionContext& context, DispatchInfo const& di)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordCommandBuffer(*cmd, context, di);
	}

	void ComputeCommand::execute(ExecutionContext& context)
	{
		DispatchInfo di{
			.push_constant = _pc,
			.extent = _dispatch_size.value(),
		};
		execute(context, di);
	}

	Executable ComputeCommand::with(DispatchInfo const& di)
	{
		using namespace vk_operators;
		return [this, di](ExecutionContext& ctx)
		{
			DispatchInfo _di{
				.push_constant = di.push_constant.hasValue() ? di.push_constant : _pc,
				.extent = di.extent != makeZeroExtent3D() ? di.extent : _dispatch_size.value(),
			};
			execute(ctx, _di);
		};
	}

	bool ComputeCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= ShaderCommand::updateResources(ctx);

		return res;
	}
}