#include "ShaderCommand.hpp"

namespace vkl
{
	void ShaderCommand::recordPushConstant(CommandBuffer& cmd, ExecutionContext& context, PushConstant const& pc)
	{
		if (pc.hasValue())
		{
			VkShaderStageFlags pc_stages = 0;
			for (const auto& pc_range : _pipeline->program()->instance()->pushConstantRanges())
			{
				pc_stages |= pc_range.stageFlags;
			}
			vkCmdPushConstants(cmd, *_pipeline->program()->instance()->pipelineLayout(), pc_stages, 0, (uint32_t)pc.size(), pc.data());
		}
	}

	void ShaderCommand::recordBindings(CommandBuffer& cmd, ExecutionContext& context)
	{
		_pipeline->waitForInstanceCreationIFN();
		const VkPipelineBindPoint bp = _pipeline->instance()->binding();
		vkCmdBindPipeline(cmd, bp, *_pipeline->instance());
		context.keppAlive(_pipeline->instance());

		DescriptorSetsManager& bound_sets = [&]() -> DescriptorSetsManager& {
			if(bp == VK_PIPELINE_BIND_POINT_GRAPHICS)
				return context.graphicsBoundSets();
			else if(bp == VK_PIPELINE_BIND_POINT_COMPUTE)
				return context.computeBoundSets();
			else
				return context.rayTracingBoundSets();
		}();
		_set->waitForInstanceCreationIFN();
		if(_set->instance()->exists())
		{
			bound_sets.bind(application()->descriptorBindingGlobalOptions().shader_set, _set->instance());
		}
		bound_sets.recordBinding(_pipeline->instance()->program()->pipelineLayout(), 
			[&context](std::shared_ptr<DescriptorSetAndPoolInstance> set_inst){ 
				context.keppAlive(set_inst); 
			}
		);
	}

	Resources ShaderCommand::getDescriptorSetResources(DescriptorSetAndPoolInstance& set, DescriptorSetLayout const& layout)
	{
		Resources res;
		res.reserve(layout.bindings().size());
		const auto& shader_bindings = layout.bindings();
		auto& set_bindings = set.bindings();
		size_t set_it = 0;
		for (size_t i = 0; i < shader_bindings.size(); ++i)
		{
			const VkDescriptorSetLayoutBinding& binding = shader_bindings[i];
			const uint32_t b = binding.binding;
			while (set_bindings[set_it].resolvedBinding() < b)
			{
				++set_it;
				if (set_it == set_bindings.size())
				{
					assertm(false, "Shader binding not found in bound set, not enough bindings!");
				}
			}
			ResourceBinding& resource = set_bindings[set_it];
			assertm(resource.resolvedBinding() == b, "Shader binding not found in bound set!");

			Resource r = resource.resource();
			const auto& meta = layout.metas()[i];
			r._begin_state = ResourceState2{
				.access = meta.access,
				.layout = meta.layout,
				.stage = getPipelineStageFromShaderStage2(binding.stageFlags),
			};
			if (r._end_state)
			{
				r._end_state = r._begin_state;
			}
			res.push_back(r);
		}
		return res;
	}

	Resources ShaderCommand::getBoundResources(DescriptorSetsTacker& bound_sets, size_t max_set)
	{
		Resources res;
		using namespace std::containers_operators;
		ProgramInstance & prog = *_pipeline->program()->instance();
		
		MultiDescriptorSetsLayouts const& sets = prog.reflectionSetsLayouts();

		if (max_set == 0)
		{
			max_set = sets.size();
		}

		for (size_t s = 0; s < max_set; ++s)
		{
			if (sets[s] && !sets[s]->empty())
			{
				DescriptorSetLayout const& set_layout = *sets[s];
				const auto& bound_set = bound_sets.getSet(s);
				if (!bound_set)
				{
					int _ = 0;
				}
				assertm(bound_set, "Shader binding set not bound!");
				res += getDescriptorSetResources(*bound_set, set_layout);
			}
		}
		return res;
	}

	bool ShaderCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= _pipeline->updateResources(ctx);

		if (_set)
		{
			res |= _set->updateResources(ctx);
		}

		return res;
	}
}