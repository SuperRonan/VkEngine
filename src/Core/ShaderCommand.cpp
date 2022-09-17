#include "ShaderCommand.hpp"

namespace vkl
{
	ResourceBinding::ResourceBinding(ShaderBindingDescriptor const& desc):
		_resource(MakeResource(desc.buffer, desc.view)),
		_binding(desc.binding),
		_set(desc.set),
		_name(desc.name)
	{
		if (!!desc.sampler)
			_samplers.push_back(desc.sampler);
	}

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
			if (b.isResolved())
			{
				VkWriteDescriptorSet write{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = *_desc_sets[b.resolvedSet()],
					.dstBinding = b.resolvedBinding(),
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
				else if (b.isImage() || b.isSampler())
				{
					VkDescriptorImageInfo info;
					if (!b.samplers().empty())
					{
						info.sampler = *b.samplers().front();
					}
					if (!b.images().empty())
					{
						info.imageView = *b.images().front();
						info.imageLayout = b.resource()._begin_state._layout;
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
		}

		vkUpdateDescriptorSets(_app->device(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
	}

	void ShaderCommand::resolveBindings()
	{
		// Attribute the program exposed bindings to the provided resources
		const auto& program = *_pipeline->program();
		const auto& sets = program.setLayouts();
		for (size_t s = 0; s < sets.size(); ++s)
		{
			const auto& set = *sets[s];
			const auto& bindings = set.bindings();
			for (size_t b = 0; b < bindings.size(); ++b)
			{
				const VkDescriptorSetLayoutBinding& vkb = bindings[b];
				const uint32_t set_id = (uint32_t)s;
				const uint32_t binding_id = vkb.binding;
				const DescriptorSetLayout::BindingMeta& meta = set.metas()[b];
				const std::string& shader_binding_name = meta.name;

				ResourceBinding* corresponding_resource = [&] {
					for (size_t i = 0; i < _bindings.size(); ++i)
					{
						ResourceBinding& resource_binding = _bindings[i];
						if (!resource_binding.isResolved())
						{
							if (resource_binding.resolveWithName())
							{
								const std::string name = (resource_binding.name().empty() ? resource_binding.resource().name() : resource_binding.name());
								if (name == shader_binding_name)
								{
									return &resource_binding;
								}
							}
							else
							{
								if (set_id == resource_binding.set() && binding_id == resource_binding.binding())
								{
									return &resource_binding;
								}
							}
						}
					}
					return (ResourceBinding *)nullptr;
				}();

				if (corresponding_resource)
				{
					corresponding_resource->resolve(set_id, binding_id);
					corresponding_resource->setType(vkb.descriptorType);
					corresponding_resource->resource()._begin_state = ResourceState{
						._access = meta.access,
						._layout = meta.layout,
						._stage = getPipelineStageFromShaderStage(vkb.stageFlags),
					};
					if (true)
					{
						std::cout << "Successfully resolved binding \"" << shader_binding_name << "\" to (set = " << set_id << ", binding = " << binding_id << ")\n";
					}
				}
				else
				{
					std::cerr << "Could not resolve Binding \"" << shader_binding_name << "\", (set = " << set_id << ", binding = " << binding_id << ")\n";
					assert(false);
				}
			}
		}
	}

	void ShaderCommand::declareDescriptorSetsResources()
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			const ResourceBinding& b = _bindings[i];
			if (b.isResolved())
			{
				_resources.push_back(b.resource());
			}
		}
	}

	void ShaderCommand::recordBindings(CommandBuffer& cmd, ExecutionContext& context)
	{
		vkCmdBindPipeline(cmd, _pipeline->binding(), *_pipeline);

		std::vector<VkDescriptorSet> desc_sets(_desc_sets.size());
		for (size_t i = 0; i < desc_sets.size(); ++i)	desc_sets[i] = *_desc_sets[i];
		vkCmdBindDescriptorSets(cmd, _pipeline->binding(), _pipeline->program()->pipelineLayout(), 0, (uint32_t)_desc_sets.size(), desc_sets.data(), 0, nullptr);

		if (!_push_constants_data.empty())
		{
			VkShaderStageFlags pc_stages = 0;
			for (const auto& pc_range : _pipeline->program()->pushConstantRanges())
			{
				pc_stages |= pc_range.stageFlags;
			}
			vkCmdPushConstants(cmd, _pipeline->program()->pipelineLayout(), pc_stages, 0, (uint32_t)_push_constants_data.size(), _push_constants_data.data());
		}
	}
}