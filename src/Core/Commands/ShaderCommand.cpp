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

	DescriptorSetsInstance::DescriptorSetsInstance(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_prog(ci.program),
		_bindings(ci.bindings)
	{
		resolveBindings();
	}

	DescriptorSetsInstance::~DescriptorSetsInstance()
	{
		
	}

	DescriptorSetsManager::~DescriptorSetsManager()
	{
		_prog->removeInvalidationCallbacks(this);
	}

	DescriptorSetsManager::DescriptorSetsManager(CreateInfo const& ci):
		ParentType(ci.app, ci.name),
		_prog(ci.program),
		_bindings(ci.bindings.cbegin(), ci.bindings.cend())
	{
		_prog->addInvalidationCallback({
			.callback = [&]() {destroyInstance(); },
			.id = this,
		});
	}

	void DescriptorSetsInstance::allocateDescriptorSets()
	{
		if (_desc_sets.empty())
		{
			_set_ranges.clear();
			_set_ranges.push_back(SetRange{
				.begin = 0,
				.len = 0,
			});

			const std::vector<std::shared_ptr<DescriptorSetLayout>> & set_layouts = _prog->setLayouts();
			_desc_pools.resize(set_layouts.size());
			_desc_sets.resize(set_layouts.size());
			for (size_t i = 0; i < set_layouts.size(); ++i)
			{
				if (_set_ranges.back().len == 0)
				{
					_set_ranges.back().begin = static_cast<uint32_t>(i);
				}
				if (!set_layouts[i]->empty())
				{
					_desc_pools[i] = std::make_shared<DescriptorPool>(DescriptorPool::CI{
					.app = application(),
					.name = name() + ".DescPool",
					.layout = set_layouts[i],
					.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
						});
					_desc_sets[i] = std::make_shared<DescriptorSet>(set_layouts[i], _desc_pools[i]);
					
					++_set_ranges.back().len;
				}
				else
				{
					if (_set_ranges.back().len != 0)
					{
						_set_ranges.push_back(SetRange{
							.begin = 0,
							.len = 0
						});
					}
				}
			}
		}
	}

	void DescriptorSetsInstance::writeDescriptorSets()
	{
		allocateDescriptorSets();

		const size_t N = _bindings.size();

		std::vector<VkDescriptorBufferInfo> buffers;
		buffers.reserve(N);
		std::vector<VkDescriptorImageInfo> images;
		images.reserve(N);

		std::vector<VkWriteDescriptorSet> writes;
		writes.reserve(N);

		bool update_uniform_buffer = false;
		bool update_storage_image = false;
		bool update_storage_buffer = false;
		bool update_sampled_image = false;

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
				switch (b.vkType())
				{
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					update_uniform_buffer = true;
					break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					update_storage_buffer = true;
					break;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					update_storage_image = true;
					break;
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					update_sampled_image = true;
					break;
				default:
					std::cerr << "Unknown descriptor type!" << std::endl;
				}
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
						info.sampler = *b.sampler()->instance();
					}
					else
					{
						info.sampler = VK_NULL_HANDLE;
					}
					if (b.image())
					{
						info.imageView = *b.image()->instance();
						info.imageLayout = b.resource()._begin_state._layout;
					}
					else
					{
						info.imageView = VK_NULL_HANDLE;
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
			bool wait = false;
			const auto& features12 = application()->availableFeatures().features_12;
			if (update_uniform_buffer && features12.descriptorBindingUniformBufferUpdateAfterBind == VK_FALSE)
			{
				wait = true;
			}
			if (update_storage_buffer && features12.descriptorBindingStorageBufferUpdateAfterBind == VK_FALSE)
			{
				wait = true;
			}
			if (update_storage_image && features12.descriptorBindingStorageImageUpdateAfterBind == VK_FALSE)
			{
				wait = false;
			}
			if (update_sampled_image && features12.descriptorBindingSampledImageUpdateAfterBind == VK_FALSE)
			{
				wait = true;
			}

			if (wait)
			{
				vkDeviceWaitIdle(device());
			}

			vkUpdateDescriptorSets(_app->device(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
		}

	}

	void DescriptorSetsInstance::resolveBindings()
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


				size_t corresponding_resource_id = 0;
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
									corresponding_resource_id = i;
									return &resource_binding;
								}
							}
							else
							{
								if (set_id == resource_binding.set() && binding_id == resource_binding.binding())
								{
									corresponding_resource_id = i;
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
					corresponding_resource->resource()._begin_state = ResourceState2{
						._access = meta.access,
						._layout = meta.layout,
						._stage = getPipelineStageFromShaderStage(vkb.stageFlags),
					};
					
					InvalidationCallback callback{
						.callback = [=]() {
							corresponding_resource->setUpdateStatus(false);
						},
						.id = this,
					};
					if (corresponding_resource->isBuffer())
					{
						corresponding_resource->buffer()->addInvalidationCallback(callback);
					}
					else if (corresponding_resource->isImage())
					{
						corresponding_resource->image()->addInvalidationCallback(callback);
					}
					else if (corresponding_resource->isSampler())
					{
						corresponding_resource->sampler()->addInvalidationCallback(callback);
					}

					
					if (false)
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

	void DescriptorSetsInstance::recordBindings(CommandBuffer& cmd, VkPipelineBindPoint binding)
	{
		if (!_desc_sets.empty())
		{
			std::vector<VkDescriptorSet> desc_sets;
			desc_sets.reserve(_desc_sets.size());
			for (const auto& range : _set_ranges)
			{
				desc_sets.resize(range.len);
				for (uint32_t i = 0; i < range.len; ++i)
				{
					assert(_desc_sets[i + range.begin]);
					desc_sets[i] = *_desc_sets[i + range.begin];
				}
				vkCmdBindDescriptorSets(
					cmd, 
					binding, 
					*_prog->pipelineLayout(), 
					range.begin, 
					range.len, 
					desc_sets.data(), 
					0, 
					nullptr
				);
			}
			
		}
	}

	void DescriptorSetsManager::createInstance(ShaderBindings const& common_bindings)
	{
		if (_inst)
		{
			destroyInstance();
		}
		using namespace std::containers_operators;

		std::vector<ResourceBinding> bindings = _bindings;
		std::vector<ResourceBinding> resources_common_bindings(common_bindings.begin(), common_bindings.end());
		bindings += resources_common_bindings;

		DescriptorSetsInstance::CreateInfo ci{
			.app = application(),
			.name = name(),
			.bindings = bindings,
			.program = _prog->instance(),
		};

		_inst = std::make_shared<DescriptorSetsInstance>(ci);
	}

	void DescriptorSetsManager::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			for (auto& binding : _bindings)
			{
				if (binding.isBuffer())
				{
					binding.buffer()->removeInvalidationCallbacks(this);
				}
				else if (binding.isImage())
				{
					binding.image()->removeInvalidationCallbacks(this);
				}
				binding.unResolve();
				binding.setUpdateStatus(false);
			}
			_inst = nullptr;
		}
	}

	bool DescriptorSetsManager::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		//res |= _prog->updateResources();

		if (!_inst)
		{
			createInstance(ctx.commonShaderBindings());
			res = true;
		}

		return res;
	}

	void ShaderCommand::recordPushConstant(CommandBuffer& cmd, ExecutionContext& context, PushConstant const& pc)
	{
		if (pc)
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
		vkCmdBindPipeline(cmd, _pipeline->instance()->binding(), *_pipeline->instance());

		_sets->instance()->recordBindings(cmd, _pipeline->instance()->binding());
	}

	void DescriptorSetsInstance::recordInputSynchronization(InputSynchronizationHelper& synch)
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			if (_bindings[i].isResolved() && _bindings[i].updated())
			{
				synch.addSynch(_bindings[i].resource());
			}
		}
	}


	bool ShaderCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= _pipeline->updateResources(ctx);

		res |= _sets->updateResources(ctx);

		_sets->instance()->writeDescriptorSets();

		return res;
	}
}