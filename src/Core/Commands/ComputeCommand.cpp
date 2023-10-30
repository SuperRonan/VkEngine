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
		_extent(ci.extent),
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
		context.pushDebugLabel(name());
		SynchronizationHelper synch(context);
		recordBindings(cmd, context);
		recordBoundResourcesSynchronization(context.computeBoundSets(), synch, application()->descriptorBindingGlobalOptions().shader_set + 1);
		
		const uint32_t set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::object)].set;
		const std::shared_ptr<PipelineLayout>& prog_layout = _pipeline->program()->instance()->pipelineLayout();

		{
			std::shared_ptr<DescriptorSetLayout> layout = [&]() -> std::shared_ptr<DescriptorSetLayout> {
				const auto& layouts = _program->instance()->reflectionSetsLayouts();
				if (set_index < layouts.size())
				{
					return layouts[set_index];
				}
				else
				{
					return nullptr;
				}
			}();
			if (layout)
			{
				std::set<void*> already_sync;
				for (auto& to_dispatch : di.dispatch_list)
				{
					if (!already_sync.contains(to_dispatch.set.get()))
					{
						assert(!!to_dispatch.set);
						recordDescriptorSetSynch(synch, *to_dispatch.set->instance(), *layout);
						already_sync.emplace(to_dispatch.set.get());
					}
				}
			}
		}

		synch.record();

		for (auto& to_dispatch : di.dispatch_list)
		{
			if (!to_dispatch.name.empty())
			{
				context.pushDebugLabel(to_dispatch.name);
			}
			if(to_dispatch.set)
			{
				std::shared_ptr<DescriptorSetAndPoolInstance> set = to_dispatch.set->instance();
				if (set->exists() && !set->empty())
				{
					context.computeBoundSets().bindOneAndRecord(set_index, set, prog_layout);
					context.keppAlive(set);
				}
			}
			recordPushConstant(cmd, context, to_dispatch.pc);

			const VkExtent3D workgroups = di.dispatch_threads ? getWorkgroupsDispatchSize(to_dispatch.extent) : to_dispatch.extent;
			vkCmdDispatch(cmd, workgroups.width, workgroups.height, workgroups.depth);
			if (!to_dispatch.name.empty())
			{
				context.popDebugLabel();
			}
		}

		context.keppAlive(_pipeline->instance());
		context.popDebugLabel();
	}

	void ComputeCommand::execute(ExecutionContext& context, DispatchInfo const& di)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordCommandBuffer(*cmd, context, di);
	}

	void ComputeCommand::execute(ExecutionContext& context)
	{
		Executable exe = with(SingleDispatchInfo{
			.extent = _extent.value(),
			.dispatch_threads = _dispatch_threads,
			.pc = _pc,
		});

		exe(context);
	}

	Executable ComputeCommand::with(DispatchInfo const& di)
	{
		using namespace vk_operators;
		return [this, di](ExecutionContext& ctx)
		{
			execute(ctx, di);
		};
	}

	bool ComputeCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= ShaderCommand::updateResources(ctx);

		return res;
	}
}