#include "ShaderCommand.hpp"

namespace vkl
{
	void ShaderCommandNode::clear()
	{
		ExecutionNode::clear();
		_set.reset();
		_pipeline.reset();
		_image_views_to_keep.clear();
	}

	void ShaderCommandNode::recordPushConstant(CommandBuffer& cmd, ExecutionContext& context, PushConstant const& pc)
	{
		if (pc.hasValue())
		{
			PipelineLayoutInstance & pipeline_layout = *_pipeline->program()->pipelineLayout();
			VkShaderStageFlags pc_stages = 0;
			for (const auto& pc_range : _pipeline->program()->pushConstantRanges())
			{
				pc_stages |= pc_range.stageFlags;
			}
			vkCmdPushConstants(cmd, pipeline_layout.handle(), pc_stages, 0, (uint32_t)pc.size(), pc.data());
		}
	}

	void ShaderCommandNode::recordBindings(CommandBuffer& cmd, ExecutionContext& context)
	{
		std::shared_ptr<PipelineLayoutInstance> pipeline_layout = _pipeline->program()->pipelineLayout();
		const VkPipelineBindPoint bp = _pipeline->binding();
		vkCmdBindPipeline(cmd, bp, *_pipeline);
		context.keepAlive(_pipeline);

		DescriptorSetsManager& bound_sets = [&]() -> DescriptorSetsManager& {
			if(bp == VK_PIPELINE_BIND_POINT_GRAPHICS)
				return context.graphicsBoundSets();
			else if(bp == VK_PIPELINE_BIND_POINT_COMPUTE)
				return context.computeBoundSets();
			else
				return context.rayTracingBoundSets();
		}();
		if(_set->exists())
		{
			bound_sets.bind(application()->descriptorBindingGlobalOptions().shader_set, _set);
		}
		bound_sets.recordBinding(_pipeline->program()->pipelineLayout(), 
			[&context](std::shared_ptr<DescriptorSetAndPoolInstance> set_inst){ 
				context.keepAlive(set_inst); 
			}
		);
		// We have to keep alive the ImageViews of descriptors(Images and Buffers are kept alive via the resource list)
		// This makes the node not reuseable
		// // TODO make this work
		//context.keepAlive(std::move(_image_views_to_keep));
		for (auto& ivtk : _image_views_to_keep)
		{
			context.keepAlive(std::move(ivtk));
		}
		_image_views_to_keep.clear();
	}




	ShaderCommand::ShaderCommand(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_provided_sets_layouts(ci.sets_layouts)
	{
		
	}


	void ShaderCommand::populateDescriptorSet(ShaderCommandNode & node, DescriptorSetAndPoolInstance& set, DescriptorSetLayoutInstance const& layout)
	{
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
			ResourceBinding& resource_binding = set_bindings[set_it];
			assertm(resource_binding.resolvedBinding() == b, "Shader binding not found in bound set!");
			const auto& meta = layout.metas()[i];
			ResourceState2 begin_state{
				.access = meta.access,
				.layout = meta.layout,
				.stage = getPipelineStageFromShaderStage2(binding.stageFlags),
			};
			Resource & resource = resource_binding.resource();
			// No end state

			if (resource_binding.isBuffer())
			{
				BufferUsage bu{
					.begin_state = begin_state,
					.usage = meta.usage,
				};
				for (auto& bar : resource.buffers)
				{
					bu.bari = bar.getInstance();
					if (bu.bari.buffer)
					{
						node.resources() += bu;
					}
				}
			}
			else if (resource_binding.isImage())
			{
				ImageViewUsage ivu{
					.begin_state = begin_state,
					.usage = static_cast<VkImageUsageFlags>(meta.usage),
				};
				for (auto& iv : resource.images)
				{
					if (iv)
					{
						ivu.ivi = iv->instance();
						node.resources() += ivu;
						// Don't forget to keep alive the ivi during the execution of the node
						node._image_views_to_keep.push_back(ivu.ivi);
					}
				}
			}
		}
	}

	void ShaderCommand::populateBoundResources(ShaderCommandNode & node, DescriptorSetsTacker& bound_sets, size_t max_set)
	{
		_pipeline->waitForInstanceCreationIFN();
		node._pipeline = _pipeline->instance();
		if (_set)
		{
			const uint32_t shader_set_index = application()->descriptorBindingGlobalOptions().shader_set;
			_set->waitForInstanceCreationIFN();
			node._set = _set->instance();
			bound_sets.bind(shader_set_index, node._set);
		}
		ProgramInstance & prog = *node._pipeline->program();
		
		MultiDescriptorSetsLayoutsInstances const& sets = prog.reflectionSetsLayouts();

		if (max_set == 0)
		{
			max_set = sets.size();
		}

		for (size_t s = 0; s < max_set; ++s)
		{
			if (sets[s] && !sets[s]->empty())
			{
				DescriptorSetLayoutInstance const& set_layout = *sets[s];
				const auto& bound_set = bound_sets.getSet(s);
				if (!bound_set)
				{
					int _ = 0;
				}
				assertm(bound_set, "Shader binding set not bound!");
				populateDescriptorSet(node, *bound_set, set_layout);
			}
		}
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