#include <Core/Commands/RayTracingCommand.hpp>

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
			std::string name = {};
			VkExtent3D extent;
			PushConstant pc = {};
			std::shared_ptr<DescriptorSetAndPoolInstance> set = nullptr;
			VkDeviceSize stack_size = VkDeviceSize(-1);
			BufferAndRangeInstance sbt_buffer;
			ShaderBindingTable::Regions sbt_regions;
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
				if (!tci.name.empty())
				{
					ctx.pushDebugLabel(tci.name);
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

				if (tci.pc.hasValue())
				{
					recordPushConstant(cmd, ctx, tci.pc);
				}

				if (tci.stack_size != VkDeviceSize(-1))
				{
					fp._vkCmdSetRayTracingPipelineStackSizeKHR(cmd, tci.stack_size);
				}

				ctx.keepAlive(tci.sbt_buffer.buffer);

				fp._vkCmdTraceRaysKHR(cmd, &tci.sbt_regions.raygen, &tci.sbt_regions.miss, &tci.sbt_regions.hit_group, &tci.sbt_regions.callable, tci.extent.width, tci.extent.height, tci.extent.depth);

				if (!tci.name.empty())
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

	std::shared_ptr<ExecutionNode> RayTracingCommand::getExecutionNode(RecordContext& ctx, size_t n, const TraceInfo * tis)
	{
		std::shared_ptr<RayTracingCommandNode> node = _exec_node_cache.getCleanNode<RayTracingCommandNode>([&]()
		{
			return std::make_shared<RayTracingCommandNode>(RayTracingCommandNode::CI{
				.app = application(),
				.name = {},
			});
		});
		node->setName(name());

		_pipeline->waitForInstanceCreationIFN();
		_set->waitForInstanceCreationIFN();
		const uint32_t shader_set_index = application()->descriptorBindingGlobalOptions().shader_set;
		const uint32_t invocation_set_index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::invocation)].set;
		ctx.rayTracingBoundSets().bind(shader_set_index, _set->instance());
		populateBoundResources(*node, ctx.rayTracingBoundSets(), shader_set_index + 1);
		std::shared_ptr<DescriptorSetLayoutInstance> layout = _pipeline->program()->instance()->reflectionSetsLayouts()[invocation_set_index];

		node->trace_calls.resize(n);
		for (size_t i = 0; i < n; ++i)
		{
			const TraceInfo & ti = tis[i];
			RayTracingCommandNode::TraceCallInfo & tci = node->trace_calls[i];
			tci.name = ti.name;
			tci.extent = ti.extent;
			tci.pc = ti.pc;
			tci.set = ti.set ? ti.set->instance() : nullptr;
			tci.stack_size = ti.stack_size;
			tci.sbt_buffer = BufferAndRangeInstance{.buffer = ti.sbt->buffer().buffer()->instance(), .range = ti.sbt->buffer().buffer()->instance()->fullRange()};
			tci.sbt_regions = ti.sbt->getRegions();

			if (layout)
			{
				populateDescriptorSet(*node, *ti.set->instance(), *layout);
			}

			node->resources() += BufferUsage{
				.bari = tci.sbt_buffer,
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR,
					.stage = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
				},
				.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
			};
		}

		return node;
	}

	std::shared_ptr<ExecutionNode> RayTracingCommand::getExecutionNode(RecordContext& ctx)
	{
		TraceInfo ti{
			.sbt = _sbt.get(),
			.extent = *_extent,
		};
		return getExecutionNode(ctx, 1, &ti);
	}

	Executable RayTracingCommand::with(TraceInfo const& ti)
	{
		return [this, ti](RecordContext & ctx)
		{
			return getExecutionNode(ctx, 1, &ti);
		};
	}
}