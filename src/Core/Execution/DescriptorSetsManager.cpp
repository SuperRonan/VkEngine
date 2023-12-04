#include "DescriptorSetsManager.hpp"
#include <algorithm>

namespace vkl
{

	DescriptorSetAndPoolInstance::DescriptorSetAndPoolInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_bindings(ci.bindings),
		_layout(ci.layout)
	{
		sortBindings();
		assert(checkIntegrity());
		allocateDescriptorSet();
		installInvalidationCallbacks();
	}

	void DescriptorSetAndPoolInstance::installInvalidationCallback(ResourceBinding& binding, Callback& cb)
	{
		if (binding.isBuffer())
		{
			if (binding.buffer())
			{
				binding.buffer()->addInvalidationCallback(cb);
			}
			else
			{
				assert(_allow_null_bindings);
				binding.setUpdateStatus(true);
			}
		}
		else if (binding.isImage() || binding.isSampler())
		{
			int null_desc = 0;
			if (binding.isImage())
			{
				if (binding.image())
				{
					binding.image()->addInvalidationCallback(cb);
				}
				else
				{
					assert(_allow_null_bindings);
					++null_desc;
				}
			}
			else if (binding.isSampler())
			{
				if (binding.sampler())
				{
					binding.sampler()->addInvalidationCallback(cb);
				}
				else
				{
					assert(_allow_null_bindings);
					++null_desc;
				}
			}
			if (binding.isImage() && binding.isSampler() && null_desc == 2)
			{
				binding.setUpdateStatus(true);
			}
		}
		else
		{
			assert(false);
		}
	}

	void DescriptorSetAndPoolInstance::removeInvalidationCallbacks(ResourceBinding& binding)
	{
		if (binding.isBuffer())
		{
			if (binding.buffer())
			{
				binding.buffer()->removeInvalidationCallbacks(this);
			}
			else
			{
				assert(_allow_null_bindings);
			}
		}
		else if (binding.isImage())
		{
			if (binding.image())
			{
				binding.image()->removeInvalidationCallbacks(this);
			}
			else
			{
				assert(_allow_null_bindings);
			}
		}
		else if (binding.isSampler())
		{
			if (binding.sampler())
			{
				binding.sampler()->removeInvalidationCallbacks(this);
			}
			else
			{
				assert(_allow_null_bindings);
			}
		}
		else
		{
			assert(false);
		}
	}

	DescriptorSetAndPoolInstance::~DescriptorSetAndPoolInstance()
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			ResourceBinding& binding = _bindings[i];
			if (!binding.isNull())
			{
				removeInvalidationCallbacks(binding);
			}
		}

	}

	size_t DescriptorSetAndPoolInstance::findBindingIndex(uint32_t b)const
	{
		size_t begin = 0, end = _bindings.size();
		size_t res = end / 2;
		while (true)
		{
			const uint32_t rb = _bindings[res].resolvedBinding();
			if (rb == b)
			{
				break;
			}
			else if (b < rb)
			{
				end = res;
			}
			else
			{
				begin = res + 1;
			}
			if (begin == end)
			{
				res = -1;
				break;
			}
			res = begin + (end - begin) / 2;
		}
		return res;
	}

	void DescriptorSetAndPoolInstance::sortBindings()
	{
		std::sort(_bindings.begin(), _bindings.end(), [](ResourceBinding const& a, ResourceBinding const& b){return a.resolvedBinding() < b.resolvedBinding();});
		if (_layout && _bindings.size() != _layout->bindings().size())
		{
			assert(false);
			ResourceBindings tmp = _bindings;
			_bindings.resize(_layout->bindings().size());
			size_t j = 0;
			for (size_t i = 0; i < _bindings.size(); ++i)
			{
				const uint32_t lb = _layout->bindings()[i].binding;
				ResourceBinding * corresponding_binding = [&]() {
					ResourceBinding* res = nullptr;
					while (j != tmp.size())
					{
						const uint32_t jb = tmp[j].resolvedBinding();
						if (jb == lb)
						{
							res = tmp.data() + j;
							break;
						}
						else if (jb > lb)
						{
							break;
						}
						++j;
					}
					return res;
				}();
				if (corresponding_binding)
				{
					_bindings[i] = *corresponding_binding;
					_bindings[i].setUpdateStatus(true);
				}
				else
				{
					_bindings[i].resolve(lb);
					_bindings[i].setType(_layout->bindings()[i].descriptorType);

					_bindings[i].setUpdateStatus(false);
				}
			}
		}
	}

	void DescriptorSetAndPoolInstance::installInvalidationCallbacks()
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			ResourceBinding & binding = _bindings[i];
			if (!binding.isNull())
			{
				Callback cb{
					.callback = [i,  this]() {
						_bindings[i].setUpdateStatus(false);
					},
					.id = this,
				};
				installInvalidationCallback(binding, cb);
			}
		}
	}

	bool DescriptorSetAndPoolInstance::checkIntegrity()const
	{
		bool res = true;
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			assert(_bindings[i].isResolved());

			res &= _bindings[i].vkType() != VK_DESCRIPTOR_TYPE_MAX_ENUM;
			assert(res);

			if(i != _bindings.size() -1)
			{
				if (_bindings[i].resolvedBinding() >= _bindings[i + 1].resolvedBinding())
				{
					res &= false;
					assert(false);
				}
			}
			
		}
		return res;
	}

	void DescriptorSetAndPoolInstance::allocateDescriptorSet()
	{
		if (!_set && !!_layout)
		{
			_pool = std::make_shared<DescriptorPool>(DescriptorPool::CI{
				.app = application(),
				.name = name() + ".pool",
				.layout = _layout,
				.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			});

			_set = std::make_shared<DescriptorSet>(DescriptorSet::CI{
				.app = application(),
				.name = name() + ".set",
				.layout = _layout,
				.pool = _pool,
			});
		}
	}

	void DescriptorSetAndPoolInstance::writeDescriptorSet(UpdateContext * context)
	{
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

		const bool can_write_null = application()->availableFeatures().robustness2_ext.nullDescriptor;
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			ResourceBinding& b = _bindings[i];
			assert(b.isResolved());
			if (!b.updated())
			{
				bool do_write = false;
				VkWriteDescriptorSet write{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = *_set,
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
					assert(b.resource()._buffer_range.hasValue());
					Buffer::Range range = b.resource()._buffer_range.value();
					if (range.len == 0)
					{
						range.len = VK_WHOLE_SIZE;
					}
					VkDescriptorBufferInfo info{
						.buffer = *b.buffer()->instance(),
						.offset = range.begin,
						.range = range.len,
					};
					do_write = true;
					if (do_write)
					{
						buffers.push_back(info);
						write.pBufferInfo = &buffers.back();
					}
				}
				else if (b.isImage() || b.isSampler())
				{
					VkDescriptorImageInfo info{
						.sampler = VK_NULL_HANDLE,
						.imageView = VK_NULL_HANDLE,
						.imageLayout = b.resource()._begin_state.layout,
					};
					
					if (b.isSampler() && b.sampler())
					{
						info.sampler = b.sampler()->instance()->handle();
						do_write = true;
					}

					
					if (b.isImage() && b.image())
					{
						info.imageView = b.image()->instance()->handle();
						do_write = true;
						if (info.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
						{
							assert(false);
						}
					}

					if (b.vkType() == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && !b.sampler() && b.image())
					{
						assert(false);
					}

					do_write |= can_write_null;
					if (b.isSampler() && info.sampler == VK_NULL_HANDLE)
					{
						// null descriptor does not work with samplers
						do_write = false;
					}
					if (do_write)
					{
						images.push_back(info);
						write.pImageInfo = &images.back();
					}
				}
				else
				{
					assert(false);
				}
				if (do_write)
				{
					writes.push_back(write);
				}
				b.setUpdateStatus(true);
			}
		}

		// TODO delegate the updates to the context
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

	void DescriptorSetAndPoolInstance::setBinding(ResourceBinding const& binding)
	{
		auto it = _bindings.begin();
		while (it != _bindings.end())
		{
			const uint32_t b = it->resolvedBinding();
			if (b == binding.resolvedBinding())
			{
				// Replace current binding
				{
					ResourceBinding & old = *it;
					if (!old.isNull())
					{
						removeInvalidationCallbacks(old);
					}
				}

				if (it->isBuffer())
				{
					it->resource()._buffer = binding.resource()._buffer;
					it->resource()._buffer_range = binding.resource()._buffer_range;
				}
				else
				{
					if (it->isImage())
					{
						it->resource()._image = binding.resource()._image;
					}
					if (it->isSampler())
					{
						it->sampler() = binding.sampler();
					}
				}
				break;
			}
			else if (b < binding.resolvedBinding())
			{
				++it;
			}
			else
			{
				assert(false);
			}
		}
		if (it == _bindings.end())
		{
			assert(false);
			//it = _bindings.insert(it, binding);
		}

		if (it->isNull())
		{
			it->setUpdateStatus(true);
		}
		else
		{
			size_t i = it - _bindings.begin();
			Callback cb{
				.callback = [i,  this]() {
					_bindings[i].setUpdateStatus(false);
				},
				.id = this,
			};
			installInvalidationCallback(*it, cb);

			it->setUpdateStatus(false);
		}


		assert(checkIntegrity());
	}









	DescriptorSetAndPool::DescriptorSetAndPool(CreateInfo const& ci):
		ParentType(ci.app, ci.name),
		_layout(ci.layout),
		_prog(ci.program),
		_target_set(ci.target_set)
	{
		_bindings.reserve(ci.bindings.size());
		for (size_t i = 0; i < ci.bindings.size(); ++i)
		{
			_bindings.push_back(ci.bindings[i]);
		}
		if (_prog)
		{
			_prog->addInvalidationCallback({
				.callback = [&]() {destroyInstance(); },
				.id = this,
			});

			if (_target_set == uint32_t(- 1))
			{
				_target_set = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::shader)].set;
			}
		}
		if (!_layout)
		{
			assert(!!_prog);
			_layout_from_prog = true;
		}
	}

	DescriptorSetAndPool::~DescriptorSetAndPool()
	{
		if (_prog)
		{
			_prog->removeInvalidationCallbacks(this);
		}
	}

	void DescriptorSetAndPool::destroyInstance()
	{
		waitForInstanceCreationIFN();
		assert(!!_inst);
		callInvalidationCallbacks();
		_inst = nullptr;
		
	}

	ResourceBindings DescriptorSetAndPool::resolveBindings(AsynchTask::ReturnType & result)
	{
		ResourceBindings res = {};

		if (_layout_from_prog)
		{
			assert(_prog);
			for (size_t j = 0; j < _bindings.size(); ++j)
			{
				_bindings[j].unResolve();
			}

			_layout = _prog->instance()->setsLayouts()[_target_set];
			if(_layout)
			{
				for (size_t i = 0; i < _layout->bindings().size(); ++i)
				{
					const VkDescriptorSetLayoutBinding & vkb = _layout->bindings()[i];
					const DescriptorSetLayout::BindingMeta & meta = _layout->metas()[i];
			
					size_t corresponding_resource_index = [&]() {
						for (size_t j = 0; j < _bindings.size(); ++j)
						{
							if (!_bindings[j].isResolved())
							{
								if (_bindings[j].resolveWithName())
								{
									const std::string name = (_bindings[j].name().empty() ? _bindings[j].resource().name() : _bindings[j].name());
									if (name == meta.name)
									{
										return j;
									}
								}
								else
								{
									if (_bindings[j].binding() == vkb.binding)
									{
										return j;
									}
								}
							}
						}
						return size_t(-1);
					}();

					if (corresponding_resource_index != size_t(-1))
					{
						ResourceBinding & resource = _bindings[corresponding_resource_index];
						resource.resolve(vkb.binding);
						resource.setType(vkb.descriptorType);
						resource.resource()._begin_state = ResourceState2{
							.access = meta.access,
							.layout = meta.layout,
							.stage = getPipelineStageFromShaderStage(vkb.stageFlags),
						};
						resource.setUpdateStatus(false);

						res.push_back(resource);
					}
					else
					{
						// Unfortunatelly, we have can't retry just this task, we would have to retry from the shader compilation
						result.success = false;
						result.can_retry = false;
						result.error_title = "DescriptorSet Binding Resolution Error"s;
						result.error_message += 
							"In DescriptorSet "s + name() + ":\n"s +
							"Layout: "s + _layout->name() + "\n"s +
							"Could not resolve Binding \""s + meta.name + "\", (set = "s + std::to_string(_target_set) + ", binding = "s + std::to_string(vkb.binding) + ")\n"s;
					}
				}
			}
		}
		else
		{
			const bool fill_missing_bindings_with_null = _allow_missing_bindings;

			if (_bindings.size() != _layout->bindings().size())
			{
				if(!fill_missing_bindings_with_null)
				{
					result = AsynchTask::ReturnType{
						.success = false,
						.can_retry = false,
						.error_title = "DescriptorSet filling missing bindings"s,
						.error_message = 
							"In DescriptorSet "s + name() + ":\n"s +
							"Layout: "s + _layout->name() + "\n"s + 
							"Not enough bindings provided ("s + std::to_string(_bindings.size()) + " vs "s + std::to_string(_layout->bindings().size()) + ")"s,
					};
					return {};
				}
				// Fill missing bindings
				if (_bindings.size() < _layout->bindings().size())
				{
					ResourceBindings new_bindings;
					new_bindings.resize(_layout->bindings().size());

					size_t j = 0;
					for (size_t i = 0; i < _layout->bindings().size(); ++i)
					{
						const uint32_t b = _layout->bindings()[i].binding;
						
						const size_t b_index = [&]() -> size_t {
							while (j != -1)
							{
								if (j == _bindings.size())
								{
									j = -1;
									return j;
								}
								else if (_bindings[j].binding() == b)
								{
									return j;
								}
								else if (_bindings[j].binding() < b)
								{
									++j;
								}
								else // if(_bindings[j].binding() > b)
								{
									return -1;
								}
							}
							return j;
						}();

						if (b_index != -1)
						{
							new_bindings[i] = _bindings[b_index];
						}
						else
						{
							Binding null_binding{
								.binding = _layout->bindings()[i].binding,
							};							
							new_bindings[i] = null_binding;
							new_bindings[i].resource()._begin_state = ResourceState2{
								.access = _layout->metas()[i].access,
								.layout = _layout->metas()[i].layout,
								.stage = getPipelineStageFromShaderStage2(_layout->bindings()[i].stageFlags),
							};
							new_bindings[i].setType(_layout->bindings()[i].descriptorType);
						}
					}

					_bindings = new_bindings;
				}
			}
			
			for (size_t j = 0; j < _bindings.size(); ++j)
			{
				_bindings[j].resolve(_bindings[j].binding());
				const auto& meta = _layout->metas()[j];
				const auto& vkb = _layout->bindings()[j];
				_bindings[j].setType(vkb.descriptorType);
				_bindings[j].resource()._begin_state.layout = meta.layout;
			}
			
			res = _bindings;
		}

		return res;
	}

	bool DescriptorSetAndPool::updateResources(UpdateContext& context)
	{
		bool res = false;

		if (!_inst)
		{	
			res = true;
			waitForInstanceCreationIFN();
			std::vector<std::shared_ptr<AsynchTask>> dependencies;

			if (_layout_from_prog)
			{
				assert(_prog);
				if (_prog->creationTask())
				{
					dependencies.push_back(_prog->creationTask());
				}
			}

			_create_instance_task = std::make_shared<AsynchTask>(AsynchTask::CI{
				.name = "Creating DescriptorSetAndPool " + name(),
				.priority = TaskPriority::ASAP(),
				.lambda = [this]()
				{
					AsynchTask::ReturnType task_res{
						.success = true,
					};
					ResourceBindings instance_bindings = resolveBindings(task_res);

					if (task_res.success)
					{
						_inst = std::make_shared<DescriptorSetAndPoolInstance>(DescriptorSetAndPoolInstance::CI{
							.app = application(),
							.name = name(),
							.layout = _layout,
							.bindings = instance_bindings,
						});

						_inst->writeDescriptorSet(nullptr);
					}


					return task_res;
				},
				.dependencies = dependencies,
			});
			application()->threadPool().pushTask(_create_instance_task);
		}
		else
		{
			_inst->writeDescriptorSet(&context);
		}
		

		return res;
	}


	void DescriptorSetAndPool::setBinding(ShaderBindingDescription const& binding)
	{
		auto it = _bindings.begin();
		ResourceBinding rb = binding;
		rb.resolve(rb.binding());
		// Bindings are not sorted !!!
		// TODO sort when resolve
		while (it != _bindings.end())
		{
			const uint32_t b = it->binding();
			if (b == binding.binding)
			{
				*it = rb;
				break;
			}
			else 
			{
				++it;
			}

		}

		if (it == _bindings.end()) // Not found
		{
			it = _bindings.insert(it, rb);
		}

		if (_inst)
		{
			_inst->setBinding(*it);
		}
	}


	void DescriptorSetAndPool::waitForInstanceCreationIFN()
	{
		if (_create_instance_task)
		{
			_create_instance_task->waitIFN();
			assert(_create_instance_task->isSuccess());
			_create_instance_task = nullptr;
		}
	}




	DescriptorSetsTacker::DescriptorSetsTacker(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_pipeline_binding(ci.pipeline_binding),
		_bound_descriptor_sets(application()->deviceProperties().props.limits.maxBoundDescriptorSets, nullptr)
	{
	
	}

	const std::shared_ptr<DescriptorSetAndPoolInstance>& DescriptorSetsTacker::getSet(uint32_t s)const
	{
		assert(s < _bound_descriptor_sets.size());
		return _bound_descriptor_sets[s];
	}

	void DescriptorSetsTacker::bind(uint32_t binding, std::shared_ptr<DescriptorSetAndPoolInstance> const& set)
	{
		assert(binding < _bound_descriptor_sets.size());
		_bound_descriptor_sets[binding] = set;
	}




	DescriptorSetsManager::DescriptorSetsManager(CreateInfo const& ci):
		DescriptorSetsTacker(DescriptorSetsTacker::CI{
			.app = ci.app,
			.name = ci.name,
			.pipeline_binding = ci.pipeline_binding,
		}),
		_cmd(ci.cmd),
		_vk_sets(_bound_descriptor_sets.size(), VK_NULL_HANDLE)
	{
		_bindings_ranges.reserve(_bound_descriptor_sets.size());
	}

	void DescriptorSetsManager::setCommandBuffer(std::shared_ptr<CommandBuffer> const& cmd)
	{
		_cmd = cmd;
		std::fill(_bound_descriptor_sets.begin(), _bound_descriptor_sets.end(), nullptr);
		std::fill(_vk_sets.begin(), _vk_sets.end(), VK_NULL_HANDLE);
		_bindings_ranges.clear();
	}

	void DescriptorSetsManager::bind(uint32_t binding, std::shared_ptr<DescriptorSetAndPoolInstance> const& set)
	{
		DescriptorSetsTacker::bind(binding, set);
		// TODO sorted insertion + merge 
		_bindings_ranges.push_back(Range32u{.begin = binding, .len = 1});
	}

	void DescriptorSetsManager::recordBinding(std::shared_ptr<PipelineLayout> const& layout, PerBindingFunction const& func)
	{
		const bool bind_all = true;
		if (bind_all)
		{
			auto & sets_layouts = layout->setsLayouts();
			uint32_t bind_begin = 0;
			uint32_t bind_len = 0;
			auto vk_bind = [&]()
			{
				vkCmdBindDescriptorSets(*_cmd, _pipeline_binding, *layout, bind_begin, bind_len, _vk_sets.data() + bind_begin, 0, nullptr);
			};
			for (size_t s = 0; s < sets_layouts.size(); ++s)
			{
				if (!!sets_layouts[s] && !!_bound_descriptor_sets[s])
				{
					if (bind_len == 0)
					{
						bind_begin = static_cast<uint32_t>(s);
						bind_len = 1;
					}
					else
					{
						++bind_len;
					}
					if (!!func)
					{
						func(_bound_descriptor_sets[s]);
					}
					_vk_sets[s] = _bound_descriptor_sets[s]->set()->handle();
					
				}
				else if(bind_len != 0)
				{
					vk_bind();
					bind_len = 0;
				}
			}
			if (bind_len != 0)
			{
				vk_bind();
				bind_len = 0;
			}
			_bindings_ranges.clear();
		}
		else
		{
			std::sort(_bindings_ranges.begin(), _bindings_ranges.end(), [](Range32u const& a, Range32u const& b){return a.begin < b.begin;});
			for (const auto r : _bindings_ranges)
			{
				for (uint32_t i = 0; i < r.len; ++i)
				{
					_vk_sets[i + r.begin] = _bound_descriptor_sets[i + r.begin]->set()->handle();
					if (func)
					{
						func(_bound_descriptor_sets[i + r.begin]);
					}
				}
				vkCmdBindDescriptorSets(*_cmd, _pipeline_binding, *layout, r.begin, r.len, _vk_sets.data() + r.begin, 0, nullptr);
			}
			_bindings_ranges.clear();
		}
	}

	void DescriptorSetsManager::bindOneAndRecord(uint32_t binding, std::shared_ptr<DescriptorSetAndPoolInstance> const& set, std::shared_ptr<PipelineLayout> const& layout)
	{
		assert(binding < _bound_descriptor_sets.size());
		assert(!!set);
		_bound_descriptor_sets[binding] = set;
		_vk_sets[binding] = set->set()->handle();
		vkCmdBindDescriptorSets(*_cmd, _pipeline_binding, *layout, binding, 1, _vk_sets.data() + binding, 0, nullptr);
	}
}