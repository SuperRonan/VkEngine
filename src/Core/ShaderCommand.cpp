#include "ShaderCommand.hpp"

namespace vkl
{
	ResourceBinding::ResourceBinding(ShaderBindingDescription const& desc):
		_resource(MakeResource(desc.buffer, desc.view)),
		_binding(desc.binding),
		_set(desc.set),
		_name(desc.name)
	{
		if (!!desc.sampler)
			_sampler = desc.sampler;
	}

	DescriptorSetsManager::~DescriptorSetsManager()
	{
		for (auto& binding : _bindings)
		{
			if (binding.isBuffer())
			{
				binding.buffer()->removeInvalidationCallbacks(this);
			}
			else if(binding.isImage())
			{
				binding.image()->removeInvalidationCallbacks(this);
			}
		}
	}

	void DescriptorSetsManager::invalidateDescriptorSets()
	{
		_desc_pools.clear();
		_desc_sets.clear();
		for (auto& binding : _bindings)
		{
			binding.setUpdateStatus(false);
		}
	}

	void DescriptorSetsManager::allocateDescriptorSets()
	{
		if (_desc_sets.empty())
		{
			std::vector<std::shared_ptr<DescriptorSetLayout>> set_layouts = _prog->setLayouts();
			_desc_pools.resize(set_layouts.size());
			_desc_sets.resize(set_layouts.size());
			for (size_t i = 0; i < set_layouts.size(); ++i)
			{
				_desc_pools[i] = std::make_shared<DescriptorPool>(set_layouts[i]);
				_desc_sets[i] = std::make_shared<DescriptorSet>(set_layouts[i], _desc_pools[i]);
			}
		}
	}

	void DescriptorSetsManager::writeDescriptorSets()
	{
		allocateDescriptorSets();

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
			if (b.isResolved() && !b.updated())
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
					buffers.emplace_back(VkDescriptorBufferInfo{
						.buffer = *b.buffer()->instance(),
						.offset = 0,
						.range = VK_WHOLE_SIZE,
						});
					VkDescriptorBufferInfo& info = buffers.back();
					write.pBufferInfo = &info;
				}
				else if (b.isImage() || b.isSampler())
				{
					VkDescriptorImageInfo info;
					if (b.sampler())
					{
						info.sampler = *b.sampler();
					}
					if (b.image())
					{
						info.imageView = *b.image()->instance();
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
				b.setUpdateStatus(true);
			}
		}

		if (!writes.empty())
		{
			vkUpdateDescriptorSets(_app->device(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
		}

	}

	void DescriptorSetsManager::resolveBindings()
	{
		// Attribute the program exposed bindings to the provided resources
		const auto& program = *_prog;
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
					InvalidationCallback callback{
						.callback = [&]() {
							corresponding_resource->setUpdateStatus(false);
						},
						.id = this,
					};
					if (corresponding_resource->isBuffer())
					{
						corresponding_resource->buffer()->addInvalidationCallback(callback);
					}
					else if(corresponding_resource->isImage())
					{
						corresponding_resource->image()->addInvalidationCallback(callback);
					}
					corresponding_resource->resolve(set_id, binding_id);
					corresponding_resource->setType(vkb.descriptorType);
					corresponding_resource->resource()._begin_state = ResourceState2{
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

	void DescriptorSetsManager::recordBindings(CommandBuffer& cmd, VkPipelineBindPoint binding)
	{
		if (!_desc_sets.empty())
		{
			std::vector<VkDescriptorSet> desc_sets(_desc_sets.size());
			for (size_t i = 0; i < desc_sets.size(); ++i)	desc_sets[i] = *_desc_sets[i];
			vkCmdBindDescriptorSets(cmd, binding, _prog->pipelineLayout(), 0, (uint32_t)_desc_sets.size(), desc_sets.data(), 0, nullptr);
		}
	}

	void ShaderCommand::recordBindings(CommandBuffer& cmd, ExecutionContext& context)
	{
		vkCmdBindPipeline(cmd, _pipeline->binding(), *_pipeline);

		_sets.recordBindings(cmd, _pipeline->binding());
		
		if (_pc)
		{
			VkShaderStageFlags pc_stages = 0;
			for (const auto& pc_range : _pipeline->program()->pushConstantRanges())
			{
				pc_stages |= pc_range.stageFlags;
			}
			vkCmdPushConstants(cmd, _pipeline->program()->pipelineLayout(), pc_stages, 0, (uint32_t)_pc.size(), _pc.data());
		}
	}

	void DescriptorSetsManager::recordInputSynchronization(InputSynchronizationHelper& synch)
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			if (_bindings[i].isResolved() && _bindings[i].updated())
			{
				synch.addSynch(_bindings[i].resource());
			}
		}
	}


	bool ShaderCommand::updateResources()
	{
		bool res = false;

		_sets.writeDescriptorSets();

		return res;
	}
}