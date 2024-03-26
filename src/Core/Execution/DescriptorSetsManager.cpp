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

	void DescriptorSetAndPoolInstance::installInvalidationCallback(ResourceBinding& binding, Callback& cb)
	{
		Resource & rs = binding.resource();
		// For now, one element in the binding is invalidated -> all the binding is invalidated
		// TODO invalidated only the element
		// For buffers, the Dyn<Range> cannot be invalidated like this, so maybe no invalidation callback
		if (binding.isBuffer())
		{
			bool all_null = true;
			bool any_null = false;
			for (size_t i = 0; i < rs.buffers.size(); ++i)
			{
				if (rs.buffers[i].buffer)
				{
					all_null = false;
					// TODO invalidate on range change (probably in the write function)
					rs.buffers[i].buffer->addInvalidationCallback(cb);
				}
				else
				{
					any_null = true;
				}
			}

			if (any_null)
			{
				assert(_allow_null_bindings);
			}
			if(all_null)
			{
				binding.setUpdateStatus(true);
			}
		}
		else if (binding.isImage() || binding.isSampler())
		{
			int null_desc = 0;
			if (binding.isImage())
			{
				bool all_null = true;
				bool any_null = false;
				for (size_t i = 0; i < rs.images.size(); ++i)
				{
					if (rs.images[i])
					{
						all_null = false;
						rs.images[i]->addInvalidationCallback(cb);
					}
					else
					{
						any_null = true;
					}
				}

				if (any_null)
				{
					assert(_allow_null_bindings);
				}
				if(all_null)
				{
					++null_desc;
				}
			}
			if (binding.isSampler())
			{
				bool all_null = true;
				bool any_null = false;
				for (size_t i = 0; i < binding.samplers().size(); ++i)
				{
					if (binding.samplers()[i])
					{
						all_null = false;
						binding.samplers()[i]->addInvalidationCallback(cb);
					}
					else
					{
						any_null = true;
					}
				}

				if (any_null)
				{
					assert(_allow_null_bindings);
				}
				if (all_null)
				{
					++null_desc;
				}
			}
			if (binding.isImage() && binding.isSampler() && null_desc == 2)
			{
				binding.setUpdateStatus(true);
			}
		}
		else if(binding.isAS())
		{
			for (size_t i = 0; i < binding.tlas().size(); ++i)
			{
				if (binding.tlas()[i])
				{
					binding.tlas()[i]->addInvalidationCallback(cb);
				}
			}
		}
		else
		{
			NOT_YET_IMPLEMENTED;
		}
	}

	void DescriptorSetAndPoolInstance::removeInvalidationCallbacks(ResourceBinding& binding)
	{
		Resource& rs = binding.resource();
		if (binding.isBuffer())
		{
			for (size_t i = 0; i < rs.buffers.size(); ++i)
			{
				if (rs.buffers[i].buffer)
				{
					rs.buffers[i].buffer->removeInvalidationCallbacks(this);
				}
			}
		}
		if (binding.isImage())
		{
			for (size_t i = 0; i < rs.images.size(); ++i)
			{
				if (rs.images[i])
				{
					rs.images[i]->removeInvalidationCallbacks(this);
				}
			}
		}
		if (binding.isSampler())
		{
			for (size_t i = 0; i < binding.samplers().size(); ++i)
			{
				if (binding.samplers()[i])
				{
					binding.samplers()[i]->removeInvalidationCallbacks(this);
				}
			}
		}
		if (binding.isAS())
		{
			for (size_t i = 0; i < binding.tlas().size(); ++i)
			{
				if (binding.tlas()[i])
				{
					binding.tlas()[i]->removeInvalidationCallbacks(this);
				}
			}
		}
	}

	DescriptorSetAndPoolInstance::~DescriptorSetAndPoolInstance()
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			ResourceBinding& binding = _bindings[i];
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
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			// Ensure descriptor arrays are fixed, pointers to it won't be invalidated
			ResourceBinding & rb = _bindings[i];
			const VkDescriptorSetLayoutBinding & vkb = _layout->bindings()[i];
			if (rb.isBuffer())
			{
				rb.resource().buffers.resize(vkb.descriptorCount);
			}
			else if (rb.isImage() || rb.isBuffer())
			{
				if (rb.isImage())
				{
					rb.resource().images.resize(vkb.descriptorCount);
				}
				if (rb.isSampler())
				{
					rb.samplers().resize(vkb.descriptorCount);
				}
			}
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

			if (_bindings[i].isBuffer())
			{
				res &= (_bindings[i].resource().buffers.size32() <= lc);
				assert(res);
			}
			else if (_bindings[i].isSampler() || _bindings[i].isImage())
			{
				uint32_t image_count = 0;
				uint32_t sampler_count = 0;
				if (_bindings[i].isImage())
				{
					image_count = _bindings[i].resource().images.size32();
				}
				if (_bindings[i].isSampler())
				{
					sampler_count = _bindings[i].samplers().size32();
				}
				if (_bindings[i].vkType() == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				{
					res &= image_count == sampler_count;
					assert(res);
				}

				res &= (std::max(image_count, sampler_count) <= lc);
				assert(res);
			}
			assert(res);

			// Check isSorted
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
			// TODO probably scan all buffers to check any range change (e.g. the updated flag is useless for buffers?)
			if (!b.updated())
			{
				bool do_write = false;
				DescriptorWriter::WriteDestination dst{
					.set = *_set,
					.binding = b.resolvedBinding(),
					.index = 0,
					.type = b.vkType(),
				};

				if (b.isBuffer())
				{
					// For now write all elems in the binding
					// TODO later write only the updated elems
					if (b.resource().buffers)
					{
						VkDescriptorBufferInfo* infos = writer.addBuffers(dst, b.resource().buffers.size());
						bool any_null = false;
						for (size_t i = 0; i < b.resource().buffers.size(); ++i)
						{
							const BufferAndRange & bar = b.resource().buffers[i];
							if (bar.buffer)
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
				else if (b.isImage() || b.isSampler())
				{
					// For now write all elems in the binding
					// TODO later write only the updated elems
					const size_t count = std::max(b.resource().images.size(), b.samplers().size());
					if (count > 0)
					{
						VkDescriptorImageInfo * infos = writer.addImages(dst, count);
						for (size_t i = 0; i < count; ++i)
						{
							VkDescriptorImageInfo & info = infos[i];
							info.sampler = VK_NULL_HANDLE;
							info.imageView = VK_NULL_HANDLE;
							info.imageLayout = b.resource().begin_state.layout;
							bool any_null = false;
							if (b.isImage())
							{
								assert(info.imageLayout != VK_IMAGE_LAYOUT_UNDEFINED);
								assert(info.imageLayout != VK_IMAGE_LAYOUT_MAX_ENUM);
								if (b.resource().images[i])
								{
									info.imageView = b.resource().images[i]->instance()->handle();
								}
								else
									any_null = true;
							}
							if (b.isSampler())
							{
								if (b.samplers()[i])
								{
									info.sampler = b.samplers()[i]->instance()->handle();
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
					if (!b.tlas().empty())
					{
						VkAccelerationStructureKHR * p_as = writer.addTLAS(dst, b.tlas().size());
						for (size_t i = 0; i < b.tlas().size(); ++i)
						{
							if (b.tlas()[i] && b.tlas()[i]->instance())
							{
								p_as[i] = b.tlas()[i]->instance()->handle();
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
				b.setUpdateStatus(true);
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

	void DescriptorSetAndPoolInstance::setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const BufferAndRange* buffers)
	{
		ResourceBinding* found = findBinding(binding);
		assert(found);
		assert(found->isBuffer());
		auto & bb = found->resource().buffers;
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
							found->setUpdateStatus(false);
						},
						.id = this,
					});
				}
			}
			found->setUpdateStatus(false);
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
		const bool is_image = found->isImage();
		const bool is_sampler = found->isSampler();
		assert(is_image || is_sampler);
		auto & bi = found->resource().images;
		auto & bs = found->samplers();

		uint32_t binding_capacity = 0;
		if (is_image)
		{
			binding_capacity = bi.size32();
		}
		else if (is_sampler)
		{
			binding_capacity = bs.size32();
		}
		if (is_image && is_sampler)
		{
			assert(bi.size() == bs.size());
		}
		assert(views || samplers);

		if (binding_capacity >= (array_index + count))
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				const uint32_t binding_array_index = array_index + i;
				Callback cb{
					.callback = [this, found, binding_array_index]() {
						found->setUpdateStatus(false);
					},
					.id = this,
				};

				if(is_image && views)
				{
					std::shared_ptr<ImageView> & binding_view = bi[binding_array_index];
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

				if (is_sampler && samplers)
				{
					std::shared_ptr<Sampler> & binding_sampler = bs[binding_array_index];
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
			found->setUpdateStatus(false);
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
		auto & b_tlas = found->tlas();
		if (b_tlas.size32() >= (array_index + count)) // enough capacity
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				const uint32_t binding_array_index = array_index + i;
				b_tlas[i] = tlas ? tlas[i] : nullptr;
				if (b_tlas[i])
				{
					b_tlas[i]->addInvalidationCallback(Callback{
						.callback = [this, found, binding_array_index]() {
							found->setUpdateStatus(false);
						},
						.id = this,
					});
				}
			}
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
									if(!_bindings[j].name().empty())
									{
										name = _bindings[j].name();
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
						resource.resource().begin_state = ResourceState2{
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
			assert(!!_layout->instance());
			DescriptorSetLayoutInstance & layout = *_layout->instance();
			std::sort(_bindings.begin(), _bindings.end(), [](ResourceBinding const& a, ResourceBinding const& b) {return a.binding() < b.binding(); });
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
								.binding = layout.bindings()[i].binding,
							};							
							new_bindings[i] = null_binding;
							new_bindings[i].resource().begin_state = ResourceState2{
								.access = layout.metas()[i].access,
								.layout = layout.metas()[i].layout,
								.stage = getPipelineStageFromShaderStage2(layout.bindings()[i].stageFlags),
							};
							new_bindings[i].setType(layout.bindings()[i].descriptorType);
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
				assert(_bindings[j].binding() == vkb.binding);
				_bindings[j].resolve(_bindings[j].binding());
				_bindings[j].setType(vkb.descriptorType);
				_bindings[j].resource().begin_state.layout = meta.layout;
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
			const uint32_t b = it->binding();
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
		auto & bb = rb.resource().buffers;
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
		auto & bi = rb.resource().images;
		auto & bs = rb.samplers();
		
		if (views)
		{
			if (bi.size32() <= (array_index + count))
			{
				bi.resize(array_index + count);
			}
		}
		if (samplers)
		{
			if (bs.size32() <= (array_index + count))
			{
				bs.resize(array_index + count);
			}
		}
		

		for (uint32_t i = 0; i < count; ++i)
		{
			if (views)
			{
				bi[array_index + i] = views[i];
			}
			if (samplers)
			{
				bs[array_index + i] = samplers[i];
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
		auto & b_tlas = rb.tlas();
		
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