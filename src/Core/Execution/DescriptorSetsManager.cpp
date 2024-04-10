#include "DescriptorSetsManager.hpp"

#include <Core/Execution/SamplerLibrary.hpp>

#include <algorithm>
#include <string_view>

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

	DescriptorSetAndPoolInstance::~DescriptorSetAndPoolInstance()
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			_bindings[i].removeCallback(this);
		}

	}

	size_t DescriptorSetAndPoolInstance::findBindingIndex(uint32_t b)const
	{
		size_t begin = 0, end = _bindings.size();
		size_t res = end / 2;
		while (true)
		{
			const uint32_t rb = _bindings[res].resolved_binding;
			assert(rb != uint32_t(-1));
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
		std::sort(_bindings.begin(), _bindings.end(), [](ResourceBinding const& a, ResourceBinding const& b){return a.resolved_binding < b.resolved_binding;});
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			// Ensure descriptor arrays are fixed, pointers to it won't be invalidated
			ResourceBinding & rb = _bindings[i];
			const VkDescriptorSetLayoutBinding & vkb = _layout->bindings()[i];
			rb.resize(vkb.descriptorCount);
			rb.invalidateAll();
		}
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
						const uint32_t jb = tmp[j].resolved_binding;
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
					_bindings[i].resetUpdateRange();
				}
				else
				{
					_bindings[i].resolve(lb);
					_bindings[i].type = _layout->bindings()[i].descriptorType;
					_bindings[i].invalidateAll();
				}
			}
		}
	}

	void DescriptorSetAndPoolInstance::installInvalidationCallbacks()
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			ResourceBinding & binding = _bindings[i];
			{	
				if (false)
				{
					Callback cb{
						.callback = [i,  this]() {
							_bindings[i].invalidateAll();
						},
						.id = this,
					};
					binding.installCallback(cb);
				}
				else
				{
					binding.installCallbacks([i, this](uint32_t index) {
						return [i, this, index]()
						{
							_bindings[i].invalidate(Range32u{.begin = index, .len = 1});
						};
					}, this);
				}
			}
		}
	}

	bool DescriptorSetAndPoolInstance::checkIntegrity()const
	{
		bool res = true;
		if (!_layout)
		{
			return res;
		}
		res &= _bindings.size() == _layout->bindings().size();
		assert(res);
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			assert(_bindings[i].isResolved());
			
			res &= _bindings[i].vkType() != VK_DESCRIPTOR_TYPE_MAX_ENUM;
			assert(res);

			res &= _bindings[i].vkType() == _layout->bindings()[i].descriptorType;
			assert(res);

			const uint32_t lc = _layout->bindings()[i].descriptorCount;
			res &= _bindings[i].size() <= lc;
			assert(res);

			// Check isSorted
			if(i != _bindings.size() -1)
			{
				if (_bindings[i].resolved_binding >= _bindings[i + 1].resolved_binding)
				{
					res &= false;
					assert(res);
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

		DescriptorWriter _own_writer(DescriptorWriter::CI{
			.app = application(),
		});

		DescriptorWriter & writer = context ? context->descriptorWriter() : _own_writer;

		const bool can_write_null = application()->availableFeatures().robustness2_ext.nullDescriptor;
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			const VkDescriptorSetLayoutBinding & layout_binding = _layout->bindings()[i];
			ResourceBinding& b = _bindings[i];
			assert(b.isResolved());
			// TODO scan all buffers to check any range change (e.g. the updated flag is useless for buffers?)
			if (b.update_range.len != 0)
			{
				bool do_write = false;
				DescriptorWriter::WriteDestination dst{
					.set = *_set,
					.binding = b.resolved_binding,
					.index = b.update_range.begin,
					.type = b.vkType(),
				};

				if (b.isBuffer())
				{
					const uint32_t n = std::min(b.update_range.len, b.buffers.size32() - b.update_range.begin);
					if (n > 0)
					{
						VkDescriptorBufferInfo* infos = writer.addBuffers(dst, n);
						bool any_null = false;
						for (uint32_t i = 0; i < n; ++i)
						{
							const BufferAndRange & bar = b.buffers[i + b.update_range.begin];
							if (bar.buffer && bar.buffer->instance())
							{
								BufferAndRangeInstance bari = bar.getInstance();
								if (bari.range.len == 0)
								{
									bari.range.len = VK_WHOLE_SIZE;
								}
								infos[i] = VkDescriptorBufferInfo{
									.buffer = bari.buffer->handle(),
									.offset = bari.range.begin,
									.range = bari.range.len,
								};
							}
							else
							{
								any_null = true;
								infos[i] = VkDescriptorBufferInfo{
									.buffer = VK_NULL_HANDLE,
									.offset = 0,
									.range = VK_WHOLE_SIZE,
								};
							}
						}
						if (any_null)
						{
							assert(can_write_null);
						}
					}
				}
				else if (b.hasImage() || b.hasSampler())
				{
					const uint32_t n = std::min(b.update_range.len, b.images_samplers.size32() - b.update_range.begin);
					if (n > 0)
					{
						VkDescriptorImageInfo * infos = writer.addImages(dst, n);
						for (uint32_t i = 0; i < n; ++i)
						{
							VkDescriptorImageInfo & info = infos[i];
							info.sampler = VK_NULL_HANDLE;
							info.imageView = VK_NULL_HANDLE;
							info.imageLayout = b.begin_state.layout;
							bool any_null = false;
							CombinedImageSampler & cis = b.images_samplers[i + b.update_range.begin];
							if (b.hasImage())
							{
								assert(info.imageLayout != VK_IMAGE_LAYOUT_UNDEFINED);
								assert(info.imageLayout != VK_IMAGE_LAYOUT_MAX_ENUM);
								if (cis.image && cis.image->instance())
								{
									info.imageView = cis.image->instance()->handle();
								}
								else
									any_null = true;
							}
							if (b.hasSampler())
							{
								if (cis.sampler && cis.sampler->instance())
								{
									info.sampler = cis.sampler->instance()->handle();
								}
								else
								{
									// The Vulkan spec allows null descriptor (with the feature enable), EXCEPT for samplers!
									// Solution: use a default sampler
									std::shared_ptr<Sampler> const& default_sampler = application()->getSamplerLibrary().getDefaultSampler();
									info.sampler = default_sampler->instance()->handle();
								}
							}

							if (b.vkType() == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
							{

							}

							if (any_null)
							{
								assert(can_write_null);
							}
						}
					}
				}
				else if(b.isAS())
				{
					const uint32_t n = std::min(b.update_range.len, b.tlases.size32() - b.update_range.begin);
					if (n > 0)
					{
						VkAccelerationStructureKHR * p_as = writer.addTLAS(dst, n);
						for (uint32_t i = 0; i < n; ++i)
						{
							const std::shared_ptr<TLAS> & tlas = b.tlases[i + b.update_range.begin];
							if(tlas && tlas->instance())
							{
								p_as[i] = tlas->instance()->handle();
							}
							else
							{
								p_as[i] = VK_NULL_HANDLE;
							}
						}
					}
				}
				else
				{
					NOT_YET_IMPLEMENTED;
				}
				b.resetUpdateRange();
			}
		}

		if (!context)
		{
			_own_writer.record();
		}
	}

	//void DescriptorSetAndPoolInstance::setBinding(ResourceBinding const& binding)
	//{
	//	auto it = _bindings.begin();
	//	ResourceBinding * found = findBinding(binding.resolvedBinding());
	//	assert(found);
	//	//if (found)
	//	{
	//		removeInvalidationCallbacks(*found);
	//		if (found->isBuffer())
	//		{
	//			found->resource().buffers = binding.resource().buffers;
	//		}
	//		else if (found->isImage() || found->isSampler())
	//		{
	//			if (found->isImage())
	//			{
	//				found->resource().images = binding.resource().images;
	//			}
	//			if (found->isSampler())
	//			{
	//				found->samplers() = binding.samplers();
	//			}
	//		}

	//		const size_t i = found - _bindings.data();
	//		Callback cb{
	//			.callback = [i,  this]() {
	//				_bindings[i].setUpdateStatus(false);
	//			},
	//			.id = this,
	//		};
	//		installInvalidationCallback(*it, cb);
	//		found->setUpdateStatus(false);
	//	}
	//	assert(checkIntegrity());
	//}

	// A note on registered resources invalidation callbacks:
	// "this" is not enough as an id an can create a bug, the binding and array index should also be included
	// If a buffer is registered at multiple bindings / array_index, then removeInvalidationCallbacks(this) will
	// remove the cb of all registering although it should not.
	// TODO Maybe extend the cb id (add an extra uint64_t id (pack binding and array_index))

	void DescriptorSetAndPoolInstance::setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const BufferAndRange* buffers)
	{
		ResourceBinding* found = findBinding(binding);
		assert(found);
		assert(found->isBuffer());
		auto & bb = found->buffers;
		if (bb.size32() >= (array_index + count)) // enough capacity
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				const uint32_t binding_array_index = array_index + i;
				BufferAndRange & binding_bar = bb[binding_array_index];
				if (binding_bar.buffer)
				{
					binding_bar.buffer->removeInvalidationCallbacks(this);
				}
			
				binding_bar = buffers ? buffers[i] : BufferAndRange{};
				if (binding_bar.buffer)
				{
					binding_bar.buffer->addInvalidationCallback(Callback{
						.callback = [this, found, binding_array_index]() {
							found->invalidate(binding_array_index);
						},
						.id = this,
					});
				}
			}
			found->invalidate(Range32u{.begin = array_index, .len = count});
			assert(checkIntegrity());
		}
		else
		{
			// this instance will be renewed, and the desc will be written then
		}
	}

	void DescriptorSetAndPoolInstance::setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const std::shared_ptr<ImageView>* views, const std::shared_ptr<Sampler>* samplers)
	{
		ResourceBinding* found = findBinding(binding);
		assert(found);
		assert(found->hasImage() || found->hasSampler());
		auto & is = found->images_samplers;

		const uint32_t binding_capacity = found->images_samplers.size32();
		assert(views || samplers);

		if (binding_capacity >= (array_index + count))
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				const uint32_t binding_array_index = array_index + i;
				CombinedImageSampler & cis = found->images_samplers[binding_array_index];
				Callback cb{
					.callback = [this, found, binding_array_index]() {
						found->invalidate(binding_array_index);
					},
					.id = this,
				};

				if(found->hasImage() && views)
				{
					std::shared_ptr<ImageView> & binding_view = cis.image;
					if (binding_view)
					{
						binding_view->removeInvalidationCallbacks(this);
					}
					binding_view = views[i];
					if (binding_view)
					{
						binding_view->addInvalidationCallback(cb);
					}
				}

				if (found->hasSampler() && samplers)
				{
					std::shared_ptr<Sampler> & binding_sampler = cis.sampler;
					if (binding_sampler)
					{
						binding_sampler->removeInvalidationCallbacks(this);
					}
					binding_sampler = samplers[i];
					if (binding_sampler)
					{
						binding_sampler->addInvalidationCallback(cb);
					}
				}
			}
			found->invalidate(Range32u{.begin = array_index, .len = count});
			assert(checkIntegrity());
		}
		else
		{
			// this instance will be renewed, and the desc will be written then
		}
	}



	void DescriptorSetAndPoolInstance::setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const std::shared_ptr<TopLevelAccelerationStructure>* tlas)
	{
		ResourceBinding* found = findBinding(binding);
		assert(found);
		assert(found->isAS());
		if (found->tlases.size32() >= (array_index + count)) // enough capacity
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				const uint32_t binding_array_index = array_index + i;
				std::shared_ptr<TLAS> & f_tlas = found->tlases[i];
				if (f_tlas)
				{
					f_tlas->removeInvalidationCallbacks(this);
				}
				f_tlas = tlas ? tlas[i] : nullptr;
				if (f_tlas)
				{
					f_tlas->addInvalidationCallback(Callback{
						.callback = [this, found, binding_array_index]() {
							found->invalidate(binding_array_index);
						},
						.id = this,
					});
				}
			}
			found->invalidate(Range32u{.begin = array_index, .len = count});
		}
		else // this instance will be renewed, no need to write now
		{

		}
	}











	size_t DescriptorSetAndPool::Registration::hash() const
	{
		return std::hash<void*>()(set.get()) ^ std::hash<uint32_t>()(binding) ^ std::hash<uint32_t>()(array_index);
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

		if (_layout)
		{
			_layout->addInvalidationCallback({
				.callback = [&]() {destroyInstance(); },
				.id = this,
			});
		}
		else
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
		if (!_layout_from_prog)
		{
			_layout->removeInvalidationCallbacks(this);
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

			//_layout = _prog->instance()->setsLayouts()[_target_set];
			std::shared_ptr<DescriptorSetLayoutInstance> layout = _prog->instance()->setsLayouts()[_target_set];
			if(layout)
			{
				for (size_t i = 0; i < layout->bindings().size(); ++i)
				{
					const VkDescriptorSetLayoutBinding & vkb = layout->bindings()[i];
					const DescriptorSetLayoutInstance::BindingMeta & meta = layout->metas()[i];
			
					size_t corresponding_resource_index = [&]() {
						for (size_t j = 0; j < _bindings.size(); ++j)
						{
							if (!_bindings[j].isResolved())
							{
								if (_bindings[j].resolveWithName())
								{
									std::string_view name;
									if(!_bindings[j].name.empty())
									{
										name = _bindings[j].name;
									}
									else
									{
										name = _bindings[j].nameFromResourceIFP();
									}
									if (name == meta.name)
									{
										return j;
									}
								}
								else
								{
									if (_bindings[j].binding == vkb.binding)
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
						resource.type = vkb.descriptorType;
						resource.begin_state = ResourceState2{
							.access = meta.access,
							.layout = meta.layout,
							.stage = getPipelineStageFromShaderStage(vkb.stageFlags),
						};
						resource.invalidateAll();

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
			assert(!!_layout->instance());
			DescriptorSetLayoutInstance & layout = *_layout->instance();
			std::sort(_bindings.begin(), _bindings.end(), [](ResourceBinding const& a, ResourceBinding const& b) {return a.binding < b.binding; });
			if (_bindings.size() != layout.bindings().size())
			{
				if(!fill_missing_bindings_with_null)
				{
					result = AsynchTask::ReturnType{
						.success = false,
						.can_retry = false,
						.error_title = "DescriptorSet filling missing bindings"s,
						.error_message = 
							"In DescriptorSet "s + name() + ":\n"s +
							"Layout: "s + layout.name() + "\n"s + 
							"Not enough bindings provided ("s + std::to_string(_bindings.size()) + " vs "s + std::to_string(layout.bindings().size()) + ")"s,
					};
					return {};
				}
				// Fill missing bindings
				if (_bindings.size() < layout.bindings().size())
				{
					ResourceBindings new_bindings;
					new_bindings.resize(layout.bindings().size());

					size_t j = 0;
					for (size_t i = 0; i < layout.bindings().size(); ++i)
					{
						const uint32_t b = layout.bindings()[i].binding;
						// _bindings are sorted
						const size_t b_index = [&]() -> size_t {
							while (j != -1)
							{
								if (j == _bindings.size())
								{
									j = -1;
									return j;
								}
								else if (_bindings[j].binding == b)
								{
									return j;
								}
								else if (_bindings[j].binding < b)
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
								.binding = layout.bindings()[i].binding,
							};							
							new_bindings[i] = null_binding;
							new_bindings[i].begin_state = ResourceState2{
								.access = layout.metas()[i].access,
								.layout = layout.metas()[i].layout,
								.stage = getPipelineStageFromShaderStage2(layout.bindings()[i].stageFlags),
							};
							new_bindings[i].type = layout.bindings()[i].descriptorType;
						}
					}

					_bindings = new_bindings;
				}
			}
			
			assert(_bindings.size() == layout.bindings().size());
			
			for (size_t j = 0; j < _bindings.size(); ++j)
			{
				const auto& meta = layout.metas()[j];
				const auto& vkb = layout.bindings()[j];
				assert(_bindings[j].binding == vkb.binding);
				_bindings[j].resolve(_bindings[j].binding);
				_bindings[j].type = vkb.descriptorType;
				_bindings[j].begin_state.layout = meta.layout;
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
						std::shared_ptr layout = _layout_from_prog ? _prog->instance()->setsLayouts()[_target_set] : _layout->instance();
						_inst = std::make_shared<DescriptorSetAndPoolInstance>(DescriptorSetAndPoolInstance::CI{
							.app = application(),
							.name = name(),
							.layout = layout,
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

	ResourceBindings::iterator DescriptorSetAndPool::findBinding(uint32_t binding)
	{
		auto it = _bindings.begin();
		// Bindings are not sorted !!!
		// TODO sort when resolve
		while (it != _bindings.end())
		{
			const uint32_t b = it->binding;
			if (b == binding)
			{
				break;
			}
			else
			{
				++it;
			}
		}
		return it;
	}

	ResourceBinding* DescriptorSetAndPool::findBindingOrEmplace(uint32_t binding)
	{
		auto it = findBinding(binding);
		if (it == _bindings.end())
		{
			_bindings.push_back(Binding{
				.binding = binding,
				});
			it = _bindings.end() - 1;
			it->resolve(binding);
		}
		return &(*it);
	}

	void DescriptorSetAndPool::Registration::clear(uint32_t num_bindings)
	{
		if (set)
		{
			for (uint32_t i = 0; i < num_bindings; ++i)
			{
				set->clearBinding(binding + i, array_index, 1);
			}
		}
	}


	void DescriptorSetAndPool::clearBinding(uint32_t binding, uint32_t array_index, uint32_t count)
	{
		NOT_YET_IMPLEMENTED;
	}

	void DescriptorSetAndPool::setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const BufferAndRange* buffers)
	{
		ResourceBinding & rb = *findBindingOrEmplace(binding);
		auto & bb = rb.buffers;
		if (bb.size32() <= (array_index + count))
		{
			bb.resize(array_index + count);
		}
		for (uint32_t i = 0; i < count; ++i)
		{
			BufferAndRange bar = buffers ? buffers[i] : BufferAndRange{};
			bb[array_index + i] = std::move(bar);
		}
		if (_inst)
		{
			_inst->setBinding(binding, array_index, count, buffers);
		}
	}

	void DescriptorSetAndPool::setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const std::shared_ptr<ImageView>* views, const std::shared_ptr<Sampler>* samplers)
	{
		ResourceBinding & rb = *findBindingOrEmplace(binding);
		auto & cis = rb.images_samplers;
		
		if (cis.size32() <= (array_index + count))
		{
			cis.resize(array_index + count);
		}
		

		for (uint32_t i = 0; i < count; ++i)
		{
			auto & cis_ = cis[array_index + i];
			if (views)
			{
				cis_.image = views[i];
			}
			if (samplers)
			{
				cis_.sampler = samplers[i];
			}
		}

		if (_inst)
		{
			_inst->setBinding(binding, array_index, count, views, samplers);
		}
	}

	void DescriptorSetAndPool::setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const std::shared_ptr<TopLevelAccelerationStructure>* tlas)
	{
		ResourceBinding & rb = *findBindingOrEmplace(binding);
		auto & b_tlas = rb.tlases;
		
		if (b_tlas.size32() <= (array_index + count))
		{
			b_tlas.resize(array_index + count); 
		}

		for (uint32_t i = 0; i < count; ++i)
		{
			b_tlas[i] = tlas ? tlas[i] : nullptr;
		}

		if (_inst)
		{
			_inst->setBinding(binding, array_index, count, tlas);
		}
	}

	//void DescriptorSetAndPool::setBinding(ShaderBindingDescription const& binding)
	//{
	//	ResourceBinding rb = binding;
	//	rb.resolve(rb.binding());
	//	
	//	auto it = findBinding(rb.resolvedBinding());

	//	if (it == _bindings.end()) // Not found
	//	{
	//		it = _bindings.insert(it, rb);
	//	}
	//	else
	//	{
	//		*it = std::move(rb);
	//	}

	//	if (_inst)
	//	{
	//		_inst->setBinding(*it);
	//	}
	//}


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
		_bound_descriptor_sets(application()->deviceProperties().props2.properties.limits.maxBoundDescriptorSets, nullptr)
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
		std::fill(_vk_sets.begin(), _vk_sets.end(), VkDescriptorSet(VK_NULL_HANDLE));
		_bindings_ranges.clear();
	}

	void DescriptorSetsManager::bind(uint32_t binding, std::shared_ptr<DescriptorSetAndPoolInstance> const& set)
	{
		DescriptorSetsTacker::bind(binding, set);
		// TODO sorted insertion + merge 
		_bindings_ranges.push_back(Range32u{.begin = binding, .len = 1});
	}

	void DescriptorSetsManager::recordBinding(std::shared_ptr<PipelineLayoutInstance> const& layout, PerBindingFunction const& func)
	{
		const bool bind_all = true;
		if (bind_all)
		{
			auto & sets_layouts = layout->setsLayouts();
			uint32_t bind_begin = 0;
			uint32_t bind_len = 0;
			auto vk_bind = [&]()
			{
				vkCmdBindDescriptorSets(*_cmd, _pipeline_binding, layout->handle(), bind_begin, bind_len, _vk_sets.data() + bind_begin, 0, nullptr);
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
				vkCmdBindDescriptorSets(*_cmd, _pipeline_binding, layout->handle(), r.begin, r.len, _vk_sets.data() + r.begin, 0, nullptr);
			}
			_bindings_ranges.clear();
		}
	}

	void DescriptorSetsManager::bindOneAndRecord(uint32_t binding, std::shared_ptr<DescriptorSetAndPoolInstance> const& set, std::shared_ptr<PipelineLayoutInstance> const& layout)
	{
		assert(binding < _bound_descriptor_sets.size());
		assert(!!set);
		_bound_descriptor_sets[binding] = set;
		_vk_sets[binding] = set->set()->handle();
		vkCmdBindDescriptorSets(*_cmd, _pipeline_binding, layout->handle(), binding, 1, _vk_sets.data() + binding, 0, nullptr);
	}
}