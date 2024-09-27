#include <vkl/Commands/ComputeCommand.hpp>

#include <that/stl_ext/const_forward.hpp>
#include <that/core/Concepts.hpp>

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
		CommandBuffer & cmd = *ctx.getCommandBuffer();
		recordBindings(cmd, ctx);

		const uint32_t shader_set_index = application()->descriptorBindingGlobalOptions().shader_set;
		const uint32_t invocation_set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
		const std::shared_ptr<PipelineLayoutInstance>& prog_layout = _pipeline->program()->pipelineLayout();

		for (auto& to_dispatch : _dispatch_list)
		{
			if (to_dispatch.name_size != 0)
			{
				ctx.pushDebugLabel(_strings.get(Range{.begin = to_dispatch.name_begin, .len = to_dispatch.name_size}), true);
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
			recordPushConstantIFN(cmd, to_dispatch.pc_begin, to_dispatch.pc_size, to_dispatch.pc_offset);

			const VkExtent3D workgroups = to_dispatch.extent;
			
			vkCmdDispatch(cmd, workgroups.width, workgroups.height, workgroups.depth);
			
			if (to_dispatch.name_size != 0)
			{
				ctx.popDebugLabel();
			}
		}

		if (ctx.framePerfCounters())
		{
			ctx.framePerfCounters()->dispatch_calls += _dispatch_list.size();
		}


		ctx.keepAlive(_pipeline);
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
			.name = name() + ".Shader",
			.source_path = _shader_path,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.definitions = _definitions,
		});
	
		_program = std::make_shared<ComputeProgram>(ComputeProgram::CI{
			.app = application(),
			.name = name() + ".Program",
			.sets_layouts = _provided_sets_layouts,
			.shader = shader,
		});

		_pipeline = std::make_shared<ComputePipeline>(ComputePipeline::CI{
			.app = application(),
			.name = name() + ".Pipeline",
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

	void ComputeCommand::DispatchInfo::clear()
	{
		ShaderCommandList::clear();
		dispatch_threads = false;
		pc_offset = 0;
		pc_begin = 0;
		pc_size = 0;
		pc_offset = 0;
		dispatch_list.clear();
	}

	struct ComputeCommandTemplateProcessor
	{
		template <that::concepts::UniversalReference<ComputeCommand::DispatchInfo::CallInfo> CallInfoRef>
		static void DispatchInfo_PushBack(ComputeCommand::DispatchInfo & that, CallInfoRef && info)
		{
			that.dispatch_list.push_back(ComputeCommand::MyDispatchCallInfo{
				.name_size = static_cast<uint32_t>(info.name.size()),
				.pc_size = info.pc_size,
				.pc_offset = info.pc_offset,
				.extent = info.extent,
				.set = std::forward<std::shared_ptr<DescriptorSetAndPool>>(info.set),
			});
			ComputeCommand::MyDispatchCallInfo & td = that.dispatch_list.back();
			if (!info.name.empty())
			{
				td.name_begin = that._strings.pushBack(info.name, true);
			}
			if (info.pc_data && td.pc_size != 0)
			{
				td.pc_begin = that._data.pushBack(info.pc_data, td.pc_size);
			}
		}


		template <that::concepts::UniversalReference<ComputeCommand::DispatchInfo> DispatchInfoRef>
		static std::shared_ptr<ExecutionNode> getExecutionNode(ComputeCommand & that, RecordContext& ctx, DispatchInfoRef&& di)
		{
			constexpr const bool is_const = std::is_const<DispatchInfoRef>::value;
			
			std::shared_ptr<ComputeCommandNode> node = that.geExecutionNodeCommon();
			that._pipeline->waitForInstanceCreationIFN();
			that._set->waitForInstanceCreationIFN();

			const uint32_t shader_set_index = that.application()->descriptorBindingGlobalOptions().shader_set;
			const uint32_t invocation_set_index = that.application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
			ctx.computeBoundSets().bind(shader_set_index, that._set->instance());
			that.populateBoundResources(*node, ctx.computeBoundSets(), shader_set_index + 1);

			std::shared_ptr<DescriptorSetLayoutInstance> layout = that._pipeline->program()->instance()->reflectionSetsLayouts()[invocation_set_index];

			node->_dispatch_list.resize(di.dispatch_list.size());

			//static thread_local std::set<void*> already_sync;
			//already_sync.clear();

			for (size_t i = 0; i < di.dispatch_list.size(); ++i)
			{
				auto & to_dispatch = di.dispatch_list[i];
				ComputeCommandNode::DispatchCallInfo& to_dispatch_inst = node->_dispatch_list[i];

				to_dispatch_inst.name_begin = to_dispatch.name_begin;
				to_dispatch_inst.pc_begin = to_dispatch.pc_begin;
				to_dispatch_inst.name_size = to_dispatch.name_size;
				to_dispatch_inst.pc_size = to_dispatch.pc_size;
				to_dispatch_inst.pc_offset = to_dispatch.pc_offset;
				to_dispatch_inst.extent = di.dispatch_threads ? that.getWorkgroupsDispatchSize(to_dispatch.extent) : to_dispatch.extent;
				to_dispatch_inst.set = to_dispatch.set ? to_dispatch.set->instance() : nullptr;

				//if (!already_sync.contains(to_dispatch.set.get()))
				if (layout)
				{
					assert(!!to_dispatch.set);
					that.populateDescriptorSet(*node, *to_dispatch.set->instance(), *layout);
					//already_sync.emplace(to_dispatch.set.get());
				}
			}

			node->_data = std::forward<decltype(node->_data)>(di._data);
			node->_strings = std::forward<decltype(node->_strings)>(di._strings);

			node->pc_begin = di.pc_begin;
			node->pc_size = di.pc_size;
			node->pc_offset = di.pc_offset;

			return node;
		}

		template <that::concepts::UniversalReference<ComputeCommand::SingleDispatchInfo> SDIRef>
		static Executable with_SDI(ComputeCommand& that, SDIRef&& sdi)
		{
			static thread_local ComputeCommand::DispatchInfo di;
			di.clear();
			VkExtent3D extent;
			if (sdi.extent.has_value())
			{
				extent = sdi.extent.value();
			}
			else
			{
				assert(that.getDispatchSize().hasValue());
				extent = that.getDispatchSize().value();
			}
			di.dispatch_threads = sdi.dispatch_threads.value_or(that.getDispatchThreads());
			if (sdi.pc_data && sdi.pc_size > 0)
			{
				di.setPushConstant(sdi.pc_data, sdi.pc_size, sdi.pc_offset);
			}
			di += ComputeCommand::DispatchInfo::CallInfo{
				.extent = extent,
				.set = std::forward<std::shared_ptr<DescriptorSetAndPool>>(sdi.set),
			};
			return that.with(std::move(di));
		}
	};


	void ComputeCommand::DispatchInfo::pushBack(CallInfo const& info)
	{
		ComputeCommandTemplateProcessor::DispatchInfo_PushBack<const CallInfo &>(*this, info);
	}

	void ComputeCommand::DispatchInfo::pushBack(CallInfo && info)
	{
		std::shared_ptr<DescriptorSetAndPool> set = info.set;
		ComputeCommandTemplateProcessor::DispatchInfo_PushBack<CallInfo&&>(*this, std::move(info));
	}

	std::shared_ptr<ComputeCommandNode> ComputeCommand::geExecutionNodeCommon()
	{
		std::shared_ptr<ComputeCommandNode> node = _exec_node_cache.getCleanNode<ComputeCommandNode>([&]() {
			return std::make_shared<ComputeCommandNode>(ComputeCommandNode::CI{
				.app = application(),
				.name = name(),
			});
		});
		node->setName(name());
		return node;
	}

	std::shared_ptr<ExecutionNode> ComputeCommand::getExecutionNode(RecordContext& ctx, DispatchInfo const& di)
	{
		return ComputeCommandTemplateProcessor::getExecutionNode<DispatchInfo const&>(*this, ctx, di);
	}

	std::shared_ptr<ExecutionNode> ComputeCommand::getExecutionNode(RecordContext& ctx, DispatchInfo && di)
	{
		decltype(auto) res = ComputeCommandTemplateProcessor::getExecutionNode<DispatchInfo &&>(*this, ctx, std::move(di));
		di.clear();
		return res;
	}

	std::shared_ptr<ExecutionNode> ComputeCommand::getExecutionNode(RecordContext& ctx)
	{
		SingleDispatchInfo sdi{
			.extent = _extent,
			.dispatch_threads = _dispatch_threads,
			.pc_data = _pc.data(),
			.pc_size = _pc.size32(),
			.pc_offset = 0,
		};
		return with(std::move(sdi))(ctx);
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

	Executable ComputeCommand::with(DispatchInfo && di)
	{
		using namespace vk_operators;
		Executable res = [this, _di = std::move(di)] (RecordContext& ctx) mutable
		{
			return getExecutionNode(ctx, std::move(_di));
		};
		return res;
	}

	
	
	Executable ComputeCommand::with(SingleDispatchInfo const& sdi)
	{
		return ComputeCommandTemplateProcessor::with_SDI(*this, sdi);
	}

	Executable ComputeCommand::with(SingleDispatchInfo && sdi)
	{
		return ComputeCommandTemplateProcessor::with_SDI<SingleDispatchInfo&&>(*this, std::move(sdi));
	}

	bool ComputeCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;
		
		res |= ShaderCommand::updateResources(ctx);

		return res;
	}
}