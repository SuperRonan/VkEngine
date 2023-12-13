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
		recordBindings(cmd, context);
		
		const uint32_t shader_set_index = application()->descriptorBindingGlobalOptions().shader_set;
		const uint32_t invocation_set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
		const std::shared_ptr<PipelineLayoutInstance>& prog_layout = _pipeline->program()->instance()->pipelineLayout();

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
					context.computeBoundSets().bindOneAndRecord(invocation_set_index, set, prog_layout);
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
		
		if (context.framePerfCounters())
		{
			context.framePerfCounters()->dispatch_calls += di.dispatch_list.size();
		}


		context.keppAlive(_pipeline->instance());
		context.popDebugLabel();
	}

	ExecutionNode ComputeCommand::getExecutionNode(RecordContext& ctx, DispatchInfo const& di)
	{
		using namespace std::containers_operators;

		const uint32_t shader_set_index = application()->descriptorBindingGlobalOptions().shader_set;
		const uint32_t invocation_set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
		_program->waitForInstanceCreationIFN(); // Wait for the pipeline layout, and descriptor sets layouts
		_set->waitForInstanceCreationIFN();
		ctx.computeBoundSets().bind(application()->descriptorBindingGlobalOptions().shader_set, _set->instance());
		ResourcesInstances resources = getBoundResources(ctx.computeBoundSets(), shader_set_index + 1);

		{
			std::shared_ptr<DescriptorSetLayoutInstance> layout = [&]() -> std::shared_ptr<DescriptorSetLayoutInstance> {
				const auto& layouts = _program->instance()->reflectionSetsLayouts();
				if (invocation_set_index < layouts.size())
				{
					return layouts[invocation_set_index];
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
						resources += getDescriptorSetResources(*to_dispatch.set->instance(), *layout);
						already_sync.emplace(to_dispatch.set.get());
					}
				}
			}
		}

		DispatchInfo di_copy = di;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, di_copy](ExecutionContext& exec_context)
			{
				std::shared_ptr<CommandBuffer> cmd = exec_context.getCommandBuffer();
				recordCommandBuffer(*cmd, exec_context, di_copy);
			},
		};
		return res;
	}

	ExecutionNode ComputeCommand::getExecutionNode(RecordContext& ctx)
	{
		DispatchInfo di{
			.dispatch_threads = _dispatch_threads,
			.dispatch_list = {
				DispatchCallInfo{
					.extent = _extent.value(),
					.pc = _pc,
				},
			},
		};

		return getExecutionNode(ctx, di);
	}

	Executable ComputeCommand::with(DispatchInfo const& di)
	{
		using namespace vk_operators;
		Executable res = [this, di](RecordContext& ctx) -> ExecutionNode
		{
			return getExecutionNode(ctx, di);
		};
		return res;
	}

	bool ComputeCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= ShaderCommand::updateResources(ctx);

		return res;
	}
}