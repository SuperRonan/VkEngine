#include <Core/Commands/RayTracingCommand.hpp>

#include <thatlib/src/core/Concepts.hpp>
#include <thatlib/src/stl_ext/const_forward.hpp>

namespace vkl
{
	struct RayTracingCommandNode : public ShaderCommandNode
	{
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		struct TraceCallInfo
		{
			Index name_begin = 0;	
			Index pc_begin = 0;
			uint32_t name_size = 0;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			VkExtent3D extent = {};
			std::shared_ptr<DescriptorSetAndPoolInstance> set = nullptr;
			VkDeviceSize stack_size = VkDeviceSize(-1);
			BufferAndRangeInstance sbt_buffer = {};
			ShaderBindingTable::Regions sbt_regions = {};
		};
		
		RayTracingCommandNode(CreateInfo const& ci) :
			ShaderCommandNode(ShaderCommandNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		MyVector<TraceCallInfo> trace_calls;

		virtual void clear() override
		{
			ShaderCommandNode::clear();
			
			trace_calls.clear();
		}

		virtual void execute(ExecutionContext& ctx) override
		{
			const uint32_t invocation_set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
			const std::shared_ptr<PipelineLayoutInstance>& prog_layout = _pipeline->program()->pipelineLayout();
			const auto & fp = application()->extFunctions();
			CommandBuffer & cmd = *ctx.getCommandBuffer();
			recordBindings(cmd, ctx);
			for (size_t i = 0; i < trace_calls.size(); ++i)
			{
				const TraceCallInfo & tci = trace_calls[i];
				if (tci.name_size != 0)
				{
					ctx.pushDebugLabel(_strings.get(Range{.begin = tci.name_begin, .len = tci.name_size}), true);
				}

				if (tci.set)
				{
					const std::shared_ptr<DescriptorSetAndPoolInstance>& set = tci.set;
					if (set->exists() && !set->empty())
					{
						ctx.rayTracingBoundSets().bindOneAndRecord(invocation_set_index, set, prog_layout);
						ctx.keepAlive(set);
					}
				}

				recordPushConstantIFN(cmd, tci.pc_begin, tci.pc_size, tci.pc_offset);

				if (tci.stack_size != VkDeviceSize(-1))
				{
					fp._vkCmdSetRayTracingPipelineStackSizeKHR(cmd, tci.stack_size);
				}

				ctx.keepAlive(tci.sbt_buffer.buffer);

				fp._vkCmdTraceRaysKHR(cmd, &tci.sbt_regions.raygen, &tci.sbt_regions.miss, &tci.sbt_regions.hit_group, &tci.sbt_regions.callable, tci.extent.width, tci.extent.height, tci.extent.depth);

				if (tci.name_size != 0)
				{
					ctx.popDebugLabel();
				}
			}
		}
	};


	RayTracingCommand::RayTracingCommand(CreateInfo const& ci) :
		ShaderCommand(ShaderCommand::CI{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts,
		}),
		_common_shader_definitions(ci.definitions),
		_extent(ci.extent)
	{
		auto addShader = [&](RTShader const& s, VkShaderStageFlagBits stage) -> std::shared_ptr<Shader>
		{
			DefinitionsList defs = s.definitions;
			return std::make_shared<Shader>(Shader::CI{
				.app = application(),
				.name = name() + ".Shader_" + std::to_string(stage),
				.source_path = s.path,
				.stage = stage,
				.definitions = [defs, this](DefinitionsList& res)
				{
					res = defs;
					if (_common_shader_definitions.hasValue())
					{
						// Be sure to call _common_shader_definitions.value() before in the update cycle!
						res += _common_shader_definitions.getCachedValue();
					}
				},
				.hold_instance = ci.hold_instance,
			});
		};

		auto addShaders = [&](MyVector<RTShader> const& rt_shaders, VkShaderStageFlagBits stage) -> MyVector<std::shared_ptr<Shader>>
		{
			MyVector<std::shared_ptr<Shader>> res(rt_shaders.size());
			for (size_t i = 0; i < res.size(); ++i)
			{
				res[i] = addShader(rt_shaders[i], stage);
			}
			return res;
		};

		//auto addShaderGroups = [&](MyVector<RTShader> const& closest_hits, MyVector<RTShader> const& any_hits, MyVector<RTShader> const& )

		std::shared_ptr<Shader> raygen = addShader(ci.raygen, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
		
		MyVector<std::shared_ptr<Shader>> misses = addShaders(ci.misses, VK_SHADER_STAGE_MISS_BIT_KHR);
		
		MyVector<std::shared_ptr<Shader>> closest_hits = addShaders(ci.closest_hits, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
		MyVector<std::shared_ptr<Shader>> any_hits = addShaders(ci.any_hits, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
		MyVector<std::shared_ptr<Shader>> intersections = addShaders(ci.intersections, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
		MyVector<RayTracingProgram::HitGroup> hit_groups(ci.hit_groups.size());
		for (size_t i = 0; i < hit_groups.size(); ++i)
		{
			if (ci.hit_groups[i].closest_hit != uint32_t(VK_SHADER_UNUSED_KHR))
			{
				hit_groups[i].closest_hit = closest_hits[ci.hit_groups[i].closest_hit];
			}
			if (ci.hit_groups[i].any_hit != uint32_t(VK_SHADER_UNUSED_KHR))
			{
				hit_groups[i].any_hit = any_hits[ci.hit_groups[i].any_hit];
			}
			if (ci.hit_groups[i].intersection != uint32_t(VK_SHADER_UNUSED_KHR))
			{
				hit_groups[i].intersection = intersections[ci.hit_groups[i].intersection];
			}
		}

		MyVector<std::shared_ptr<Shader>> callables = addShaders(ci.callables, VK_SHADER_STAGE_CALLABLE_BIT_KHR);

		const uint32_t miss_count = misses.size32();
		const uint32_t hit_group_count = hit_groups.size32();
		const uint32_t callable_count = callables.size32();

		std::shared_ptr<RayTracingProgram> program = std::make_shared<RayTracingProgram>(RayTracingProgram::CI{
			.app = application(),
			.name = name() + ".Program",
			.raygen = std::move(raygen),
			.misses = std::move(misses),
			.hit_groups = std::move(hit_groups),
			.callables = std::move(callables),
			.sets_layouts = ci.sets_layouts,
			.hold_instance = ci.hold_instance,
		});

		_pipeline = std::make_shared<RayTracingPipeline>(RayTracingPipeline::CI{
			.app = application(),
			.name = name() + ".Pipeline",
			.program = program,
			.max_recursion_depth = ci.max_recursion_depth,
			.hold_instance = ci.hold_instance,
		});

		_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CI{
			.app = application(),
			.name = name() + ".shader_set",
			.program = program,
			.target_set = application()->descriptorBindingGlobalOptions().shader_set,
			.bindings = ci.bindings,
		});

		if (ci.create_sbt)
		{
			using SBT = ShaderBindingTable;
			_sbt = std::make_shared<SBT>(SBT::CI{
				.app = application(),
				.name = name() + ".SBT",
				.pipeline = getRTPipeline(),
				.raygen = SBT::Capacity{.count = 1, .data_size = 0},
				.miss = SBT::Capacity{.count = miss_count, .data_size = 0},
				.hit_group = SBT::Capacity{.count = hit_group_count, .data_size = 0},
				.callable = SBT::Capacity{.count = callable_count, .data_size = 0},
				.extra_buffer_usage = ci.sbt_extra_buffer_usage,
			});
		}
	}

	RayTracingCommand::~RayTracingCommand()
	{

	}

	bool RayTracingCommand::updateResources(UpdateContext& ctx)
	{
		_common_shader_definitions.value();
		bool res = ShaderCommand::updateResources(ctx);
		if (_sbt)
		{
			_sbt->updateResources(ctx);
		}
		return res;
	}

	void RayTracingCommand::TraceInfo::clear()
	{
		ShaderCommandList::clear();
		pc_offset = 0;
		calls.clear();
		sbt = nullptr;
		stack_size = VkDeviceSize(-1);
	}

	struct RayTracingCommandTemplateProcessor
	{
		template <that::concepts::UniversalReference<RayTracingCommand::TraceInfo> TraceInfoRef>
		static std::shared_ptr<ExecutionNode> getExecutionNode(RayTracingCommand& that, RecordContext& ctx, TraceInfoRef&& info)
		{
			std::shared_ptr<RayTracingCommandNode> node = that._exec_node_cache.getCleanNode<RayTracingCommandNode>([&]()
			{
				return std::make_shared<RayTracingCommandNode>(RayTracingCommandNode::CI{
					.app = that.application(),
					.name = {},
				});
			});
			node->setName(that.name());

			that._pipeline->waitForInstanceCreationIFN();
			that._set->waitForInstanceCreationIFN();
			const uint32_t shader_set_index = that.application()->descriptorBindingGlobalOptions().shader_set;
			const uint32_t invocation_set_index = that.application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
			ctx.rayTracingBoundSets().bind(shader_set_index, that._set->instance());
			that.populateBoundResources(*node, ctx.rayTracingBoundSets(), shader_set_index + 1);
			std::shared_ptr<DescriptorSetLayoutInstance> layout = that._pipeline->program()->instance()->reflectionSetsLayouts()[invocation_set_index];

			node->trace_calls.resize(info.calls.size());

			ShaderBindingTable * const common_sbt = info.sbt ? info.sbt : that._sbt.get();
			bool use_common_sbt = false;
			const ResourceState2 sbt_state = ResourceState2{
				.access = VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR,
				.stage = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			};
			const VkBufferUsageFlags2KHR sbt_usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
			
			for (size_t i = 0; i < info.calls.size(); ++i)
			{
				auto& ti = info.calls[i];
				RayTracingCommandNode::TraceCallInfo& tci = node->trace_calls[i];

				ShaderBindingTable * const sbt = ti.sbt ? ti.sbt : common_sbt;

				tci.name_begin = ti.name_begin;
				tci.pc_begin = ti.pc_begin;
				tci.name_size = ti.name_size;
				tci.pc_size = ti.pc_size;
				tci.pc_offset = ti.pc_offset;
				tci.extent = ti.extent;
				tci.sbt_buffer = sbt->buffer().getSegmentInstance();
				tci.sbt_regions = sbt->getRegions();

				if (layout)
				{
					that.populateDescriptorSet(*node, *ti.set->instance(), *layout);
				}

				if (sbt != common_sbt)
				{
					node->resources() += BufferUsage{
						.bari = tci.sbt_buffer,
						.begin_state = sbt_state,
						.usage = sbt_usage,
					};
				}
				else
				{
					use_common_sbt = true;
				}
			}
			if (use_common_sbt)
			{
				node->resources() += BufferUsage{
					.bari = common_sbt->buffer().getSegmentInstance(),
					.begin_state = sbt_state,
					.usage = sbt_usage,
				};
			}

			node->_data = std::forward<decltype(node->_data)>(info._data);
			node->_strings = std::forward<decltype(node->_strings)>(info._strings);

			node->pc_begin = info.pc_begin;
			node->pc_size = info.pc_size;
			node->pc_offset = info.pc_offset;

			return node;
		}

		template <that::concepts::UniversalReference<RayTracingCommand::TraceInfo::CallInfo> CallInfoRef>
		static void TraceInfo_pushBack(RayTracingCommand::TraceInfo & that, CallInfoRef&& ci)
		{
			using Index = ShaderCommandList::Index;
			that.calls += RayTracingCommand::MyTraceCallInfo{
				.name_size = static_cast<uint32_t>(ci.name.size()),
				.pc_size = ci.pc_size,
				.pc_offset = ci.pc_offset,
				.extent = ci.extent,
				.sbt = ci.sbt,
				.set = std::forward<std::shared_ptr<DescriptorSetAndPool>>(ci.set),
				.stack_size = ci.stack_size,
			};
			RayTracingCommand::MyTraceCallInfo & tc = that.calls.back();
			if (!ci.name.empty())
			{
				tc.name_begin = that._strings.pushBack(ci.name, true);
			}
			if (ci.pc_data && tc.pc_size != 0)
			{
				tc.pc_begin = that._data.pushBack(ci.pc_data, tc.pc_size);
			}
		}

		template <that::concepts::UniversalReference<RayTracingCommand::SingleTraceInfo> STIRef>
		static Executable with_STI(RayTracingCommand& that, STIRef&& sti)
		{
			static thread_local RayTracingCommand::TraceInfo ti;
			ti.clear();

			if (sti.pc_data && sti.pc_size != 0)
			{
				ti.setPushConstant(sti.pc_data, sti.pc_size, sti.pc_offset);
			}

			ti.sbt = sti.sbt;
			ti.stack_size = sti.stack_size;
			
			ti.calls += RayTracingCommand::MyTraceCallInfo{
				.set = std::forward<std::shared_ptr<DescriptorSetAndPool>>(sti.set),
			};

			RayTracingCommand::MyTraceCallInfo & tci = ti.calls.back();
			if (sti.extent.has_value())
			{
				tci.extent = sti.extent.value();
			}
			else
			{
				tci.extent = that._extent.value();
			}

			return that.with(std::move(ti));
		}
	};

	void RayTracingCommand::TraceInfo::pushBack(CallInfo const& ci)
	{
		RayTracingCommandTemplateProcessor::TraceInfo_pushBack<CallInfo const&>(*this, ci);
	}

	void RayTracingCommand::TraceInfo::pushBack(CallInfo && ci)
	{
		RayTracingCommandTemplateProcessor::TraceInfo_pushBack<CallInfo &&>(*this, std::move(ci));
	}

	std::shared_ptr<ExecutionNode> RayTracingCommand::getExecutionNode(RecordContext& ctx, TraceInfo const& ti)
	{
		return RayTracingCommandTemplateProcessor::getExecutionNode<TraceInfo const&>(*this, ctx, ti);
	}

	std::shared_ptr<ExecutionNode> RayTracingCommand::getExecutionNode(RecordContext& ctx, TraceInfo && ti)
	{
		decltype(auto) res = RayTracingCommandTemplateProcessor::getExecutionNode<TraceInfo&&>(*this, ctx, std::move(ti));
		ti.clear();
		return res;
	}

	std::shared_ptr<ExecutionNode> RayTracingCommand::getExecutionNode(RecordContext& ctx)
	{
		SingleTraceInfo sti{
			.extent = _extent,
			.sbt = _sbt.get(),
		};
		if (_pc.hasValue())
		{
			sti.pc_data = _pc.data();
			sti.pc_size = _pc.size32();
		}
		return with(std::move(sti))(ctx);
	}

	Executable RayTracingCommand::with(TraceInfo const& ti)
	{	
		return [this, ti](RecordContext & ctx)
		{
			return getExecutionNode(ctx, ti);
		};
	}

	Executable RayTracingCommand::with(TraceInfo && ti)
	{
		return [this, _ti = std::move(ti)](RecordContext& ctx) mutable
		{
			return getExecutionNode(ctx, std::move(_ti));
		};
	}

	Executable RayTracingCommand::with(SingleTraceInfo const& sti)
	{
		return RayTracingCommandTemplateProcessor::with_STI<SingleTraceInfo const&>(*this, sti);
	}

	Executable RayTracingCommand::with(SingleTraceInfo && sti)
	{
		return RayTracingCommandTemplateProcessor::with_STI<SingleTraceInfo &&>(*this, std::move(sti));
	}
}