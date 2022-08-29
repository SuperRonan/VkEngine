#include "ShaderCommand.hpp"

namespace vkl
{
	void ShaderCommand::writeDescriptorSets()
	{
		std::vector<std::shared_ptr<DescriptorSetLayout>> set_layouts = _pipeline->program()->setLayouts();
		_desc_pools.resize(set_layouts.size());
		_desc_sets.resize(set_layouts.size());
		for (size_t i = 0; i < set_layouts.size(); ++i)
		{
			_desc_pools[i] = std::make_shared<DescriptorPool>(set_layouts[i]);
			_desc_sets[i] = std::make_shared<DescriptorSet>(set_layouts[i], _desc_pools[i]);
		}
		const size_t N = _bindings.size();

		std::vector<VkDescriptorBufferInfo> buffers;
		buffers.reserve(N);
		std::vector<VkDescriptorImageInfo> images;
		images.reserve(N);

		std::vector<VkWriteDescriptorSet> writes;
		writes.reserve(N);

		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			ResourceBinding& b = _bindings[i];
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = *_desc_sets[b.set()],
				.dstBinding = b.binding(),
				.dstArrayElement = 0, // TODO
				.descriptorCount = 1,
				.descriptorType = b.vkType(),
			};
			if (b.isBuffer())
			{
				assert(b.buffers().size() == 1);
				buffers.emplace_back(VkDescriptorBufferInfo{
					.buffer = *b.buffers().front(),
					.offset = 0,
					.range = VK_WHOLE_SIZE,
					});
				VkDescriptorBufferInfo& info = buffers.back();
				write.pBufferInfo = &info;
			}
			else if (b.isImage())
			{
				VkDescriptorImageInfo info;
				if (!b.samplers().empty())
				{
					info.sampler = *b.samplers().front();
				}
				if (!b.images().empty())
				{
					info.imageView = *b.images().front();
					info.imageLayout = b.state()._layout;
				}
				images.push_back(info);
				write.pImageInfo = &images.back();
			}
			else
			{
				assert(false);
			}

			writes.push_back(write);
		}

		vkUpdateDescriptorSets(_app->device(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
	}

	void ShaderCommand::processBindingsList()
	{
		// Resolve named bindings
		const auto& program = *_pipeline->program();
		const auto& sets = program.setLayouts();
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			ResourceBinding& binding = _bindings[i];
			const VkDescriptorSetLayoutBinding* vkb = nullptr;
			const DescriptorSetLayout::BindingMeta* bmeta = nullptr;

			const std::string name = (binding.name().empty() ? binding.resource().name() : binding.name());
			
			const auto findBindingInSet = [&](size_t s)
			{
				const auto& set = *sets[s];
				for (size_t b = 0; b < set.bindings().size(); ++b)
				{
					const bool binding_found = [&] {
						if (binding.resolved())
						{
							return binding.binding() == set.bindings()[b].binding;
						}
						else
						{
							return name == set.metas()[b].name;
						}
					}();

					if (binding_found)
					{
						vkb = set.bindings().data() + b;
						bmeta = set.metas().data() + b;
						return true;
					}
				}
				return false;
			};

			uint32_t found_set;
			if (binding.resolved())
			{
				found_set = binding.set();
				findBindingInSet((size_t)binding.set());
			}
			else
			{
				assert(!name.empty());
				for (size_t s = 0; s < sets.size(); ++s)
				{
					const bool found = findBindingInSet(s);
					if (found)
					{
						found_set = s;
						break;
					}
				}
			}

			assert(vkb);

			binding.setType(vkb->descriptorType);
			if(!binding.resolved())
				binding.setBinding(found_set, vkb->binding);
			if (binding.name().empty())
				binding.setName(bmeta->name.empty() ? binding.resource().name() : bmeta->name);
			binding.setState(ResourceState{
				._access = bmeta->access,
				._layout = bmeta->layout,
				._stage = vkb->stageFlags,
			});
		}
	}

	void ShaderCommand::declareDescriptorSetsResources()
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			const ResourceBinding& b = _bindings[i];
			_resources.push_back(b.resource());
		}
	}

	void ShaderCommand::extractBindingsFromReflection()
	{
		const auto& program = *_pipeline->program();
		const auto& sets = program.setLayouts();
		for (size_t s = 0; s < sets.size(); ++s)
		{
			const auto& set = *sets[s];
			for (size_t b = 0; b < set.bindings().size(); ++b)
			{
				const auto& binding = set.bindings()[b];

			}
		}
	}

	void ShaderCommand::recordBindings(CommandBuffer& cmd, ExecutionContext& context)
	{
		vkCmdBindPipeline(cmd, _pipeline->binding(), *_pipeline);

		std::vector<VkDescriptorSet> desc_sets(_desc_sets.size());
		for (size_t i = 0; i < desc_sets.size(); ++i)	desc_sets[i] = *_desc_sets[i];
		vkCmdBindDescriptorSets(cmd, _pipeline->binding(), _pipeline->program()->pipelineLayout(), 0, (uint32_t)_desc_sets.size(), desc_sets.data(), 0, nullptr);

		VkPipelineStageFlags pc_stages = 0;
		for (const auto& pc_range : _pipeline->program()->pushConstantRanges())
		{
			pc_stages |= pc_range.stageFlags;
		}
		vkCmdPushConstants(cmd, _pipeline->program()->pipelineLayout(), pc_stages, 0, (uint32_t)_push_constants_data.size(), _push_constants_data.data());
	}
}