#include "ComputeCommand.hpp"

namespace vkl
{

	ComputeCommandNode::ComputeCommandNode(CreateInfo const& ci) :
		ShaderCommandNode(ShaderCommandNode::CI{
			.app = ci.app,
			.name = ci.name,
		})
	{}

	void ComputeCommandNode::clear()
	{
		ShaderCommandNode::clear();

		_dispatch_list.clear();
	}

	void ComputeCommandNode::execute(ExecutionContext& ctx)
	{
		ctx.pushDebugLabel(name());
		CommandBuffer & cmd = *ctx.getCommandBuffer();
		recordBindings(cmd, ctx);

		const uint32_t shader_set_index = application()->descriptorBindingGlobalOptions().shader_set;
		const uint32_t invocation_set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
		const std::shared_ptr<PipelineLayoutInstance>& prog_layout = _pipeline->program()->pipelineLayout();

		for (auto& to_dispatch : _dispatch_list)
		{
			if (!to_dispatch.name.empty())
			{
				ctx.pushDebugLabel(to_dispatch.name);
			}
			if (to_dispatch.set)
			{
				const std::shared_ptr<DescriptorSetAndPoolInstance> & set = to_dispatch.set;
				if (set->exists() && !set->empty())
				{
					ctx.computeBoundSets().bindOneAndRecord(invocation_set_index, set, prog_layout);
					ctx.keepAlive(set);
				}
			}
			recordPushConstant(cmd, ctx, to_dispatch.pc);

			const VkExtent3D workgroups = to_dispatch.extent;
			
			vkCmdDispatch(cmd, workgroups.width, workgroups.height, workgroups.depth);
			
			if (!to_dispatch.name.empty())
			{
				ctx.popDebugLabel();
			}
		}

		if (ctx.framePerfCounters())
		{
			ctx.framePerfCounters()->dispatch_calls += _dispatch_list.size();
		}


		ctx.keepAlive(_pipeline);
		ctx.popDebugLabel();
	}


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

	std::shared_ptr<ExecutionNode> ComputeCommand::getExecutionNode(RecordContext& ctx, DispatchInfo const& di)
	{
		std::shared_ptr<ComputeCommandNode> node = _exec_node_cache.getCleanNode<ComputeCommandNode>([&]() {
			return std::make_shared<ComputeCommandNode>(ComputeCommandNode::CI{
				.app = application(),
				.name = name(),
			});
		});

		node->setName(name());

		_pipeline->waitForInstanceCreationIFN();
		_set->waitForInstanceCreationIFN();

		const uint32_t shader_set_index = application()->descriptorBindingGlobalOptions().shader_set;
		const uint32_t invocation_set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
		ctx.computeBoundSets().bind(application()->descriptorBindingGlobalOptions().shader_set, _set->instance());
		populateBoundResources(*node, ctx.computeBoundSets(), shader_set_index + 1);

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

		node->_dispatch_list.resize(di.dispatch_list.size());

		
		//static thread_local std::set<void*> already_sync;
		//already_sync.clear();

		for (size_t i = 0; i < di.dispatch_list.size(); ++i)
		{
			const DispatchCallInfo & to_dispatch = di.dispatch_list[i];
			ComputeCommandNode::DispatchCallInfo & to_dispatch_inst = node->_dispatch_list[i];

			to_dispatch_inst.name = to_dispatch.name;
			to_dispatch_inst.extent = di.dispatch_threads ? getWorkgroupsDispatchSize(to_dispatch.extent) : to_dispatch.extent;
			to_dispatch_inst.pc = to_dispatch.pc;
			to_dispatch_inst.set = to_dispatch.set ? to_dispatch.set->instance() : nullptr;
				
			//if (!already_sync.contains(to_dispatch.set.get()))
			if (layout)
			{
				assert(!!to_dispatch.set);
				populateDescriptorSet(*node, *to_dispatch.set->instance(), *layout);
				//already_sync.emplace(to_dispatch.set.get());
			}
		}
		return node;
	}

	std::shared_ptr<ExecutionNode> ComputeCommand::getExecutionNode(RecordContext& ctx)
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
		Executable res = [this, di](RecordContext& ctx)
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