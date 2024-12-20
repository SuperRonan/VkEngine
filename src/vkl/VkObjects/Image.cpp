#include <vkl/VkObjects/Image.hpp>

namespace vkl
{
	std::atomic<size_t> ImageInstance::_instance_counter = 0;

	void ImageInstance::setVkNameIFP()
	{
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT object_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = (uint64_t)_image,
				.pObjectName = _name.data(),
			};
			_app->nameVkObjectIFP(object_name);
		}
	}

	void ImageInstance::setInitialState(size_t tid)
	{
		_states[tid] = InternalStates();
		InternalStates &is = _states[tid];

		is.states.resize(_ci.mipLevels);
		for (size_t m = 0; m < is.states.size(); ++m)
		{
			is.states[m] = {
				InternalStates::PosAndState{
					.pos = 0,
					.write_state = ResourceState2{
						.access = VK_ACCESS_2_NONE,
						.stage = VK_PIPELINE_STAGE_2_NONE,
						.layout = _ci.initialLayout,
					},
					.read_only_state = ResourceState2{
						.access = VK_ACCESS_2_NONE,
						.stage = VK_PIPELINE_STAGE_2_NONE,
						.layout = _ci.initialLayout,
					},
				},
			};
		}

	}

	void ImageInstance::create()
	{
		assert(_image == VK_NULL_HANDLE);

		VK_CHECK(vmaCreateImage(_app->allocator(), &_ci, &_vma_ci, &_image, &_alloc, nullptr), "Failed to create an image.");

		setVkNameIFP();
	}

	void ImageInstance::destroy()
	{
		assert(_image != VK_NULL_HANDLE);

		callDestructionCallbacks();

		if (ownership())
		{
			vmaDestroyImage(_app->allocator(), _image, _alloc);
		}

		_image = VK_NULL_HANDLE;
		_alloc = nullptr;
	}

	bool ImageInstance::statesAreSorted(size_t tid) const
	{
		assert(_states.contains(tid));
		const auto& states = _states.at(tid).states;
		for (size_t m = 0; m < states.size(); ++m)
		{
			for (size_t i = 1; i < states[m].size(); ++i)
			{
				if (states[m][i - 1].pos >= states[m][i].pos)
				{
					return false;
				}
			}
		}
		return true;
	}

	ImageInstance::ImageInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_ci(ci.ci),
		_vma_ci(ci.aci),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{
		create();
		setInitialState(0);
	}

	ImageInstance::ImageInstance(AssociateInfo const& ai) :
		AbstractInstance(ai.app, ai.name),
		_ci(ai.ci),
		_image(ai.image),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{
		setInitialState(0);
	}

	ImageInstance::~ImageInstance()
	{
		if (!!_image)
		{
			destroy();
		}
	}

	void ImageInstance::fillState(size_t tid, Range const& range, MyVector<StateInRange> & res) const
	{
		assert(statesAreSorted(tid));
		const uint32_t range_max_mip = range.baseMipLevel + range.levelCount;
		const uint32_t range_max_layer = range.baseArrayLayer + range.layerCount;

		// Or keep this cached in thiss
		static thread_local 
		MyVector<MyVector<StateInRange>> states_per_mip;
		size_t states_per_mips_size = range.levelCount;
		states_per_mip.resize(std::max(states_per_mips_size, states_per_mip.size()));
		for (auto& spm : states_per_mip)
		{
			spm.clear();
		}

		
		for (uint32_t m = range.baseMipLevel; m < (range.baseMipLevel + range.levelCount); ++m)
		{
			std::vector<StateInRange>& res_states = states_per_mip[m - range.baseMipLevel];
			const std::vector<InternalStates::PosAndState> & layers_states = _states.at(tid).states[m];

			assert(layers_states[0].pos == 0);
			
			for (size_t i = 0; i < layers_states.size(); ++i)
			{
				const uint32_t layers_begin = layers_states[i].pos;
				const uint32_t layers_end = [&]() {
					if (i == layers_states.size() - 1)
						return _ci.arrayLayers;
					else
						return layers_states[i+1].pos;
				}();
				
				if (range.baseArrayLayer >= layers_end)
				{
					continue;
				}
				if (layers_begin >= range_max_layer)
				{
					break;
				}

				uint32_t begin = std::max(layers_begin, range.baseArrayLayer);
				uint32_t end = std::min(layers_end, range_max_layer);
				res_states.push_back(StateInRange{
					.state = DoubleResourceState2{
						.write_state = layers_states[i].write_state,
						.read_only_state = layers_states[i].read_only_state,
					},
					.range = Range{
						.aspectMask = range.aspectMask,
						.baseMipLevel = m,
						.levelCount = 1,
						.baseArrayLayer = begin,
						.layerCount = end - begin,
					},
				});
			}
		}

		
		// Try to merge mips
		// The code is not perfect, it misses some potential merges
		for (size_t m = (states_per_mips_size - 1); m > 0; --m)
		{
			//Container<StateInRange>
			auto & mip_minus = states_per_mip[m - 1];
			auto & current_mip = states_per_mip[m];

			bool can_merge = [&]() {
				bool can_merge = false;
				if (mip_minus.size() == current_mip.size())
				{
					can_merge = true;
					for (size_t i = 0; i < mip_minus.size(); ++i)
					{
						can_merge &= (
							(mip_minus[i].state == current_mip[i].state) &&
							(mip_minus[i].range.baseArrayLayer == current_mip[i].range.baseArrayLayer) &&
							(mip_minus[i].range.layerCount == current_mip[i].range.layerCount)
							);
						if (!can_merge)
						{
							break;
						}
					}
				}
				return can_merge;
			}();

			if (can_merge)
			{
				for (size_t i = 0; i < mip_minus.size(); ++i)
				{
					mip_minus[i].range.levelCount += current_mip[i].range.levelCount;
				}
				//states_per_mip.pop_back();
				--states_per_mips_size;
			}
			else
			{
				break;
			}
		}

		// Linearize to res
		res.clear();
		//res.reserve(states_per_mips_size * states_per_mip[0].size());
		using namespace std::containers_append_operators;
		for (size_t m = 0; m < states_per_mips_size; ++m)
		{
			res += states_per_mip[m];
		}
	}

	void ImageInstance::setState(size_t tid, Range const& range, ResourceState2 const& state)
	{
		const bool state_is_readonly = accessIsReadonly2(state.access);
		const uint32_t range_max_mip = range.baseMipLevel + range.levelCount;
		const uint32_t range_max_layer = range.baseArrayLayer + range.layerCount;

		for (uint32_t m = range.baseMipLevel; m < (range.baseMipLevel + range.levelCount); ++m)
		{
			//Container<PosAndState>
			auto& layers_states = _states.at(tid).states[m];
			for (auto it = layers_states.begin(); it != layers_states.end(); ++it)
			{
				const uint32_t layers_begin = it->pos;
				const uint32_t layers_end = [&]() {
					if ((it + 1) == layers_states.end())
						return _ci.arrayLayers;
					else
						return (it + 1)->pos;
				}();

				if (range.baseArrayLayer >= layers_end) // Not yet there
				{
					continue;
				}
				if (layers_begin >= range_max_layer) // Done
				{
					break;
				}

				if (range.baseArrayLayer <= it->pos && range_max_layer >= layers_end) // it is a subset of range
				{
					if (state_is_readonly)
					{
						// Take the layout of new state
						it->read_only_state = (state | it->read_only_state);
					}
					else
					{
						it->write_state = state;
						it->read_only_state = {.layout = it->write_state.layout};
					}
				}
				else
				{
					if (range.baseArrayLayer > it->pos)
					{
						InternalStates::PosAndState new_state {
							.pos = range.baseArrayLayer,
						};
						if (state_is_readonly)
						{
							new_state.read_only_state = (state | it->read_only_state);
							new_state.write_state = it->write_state;
						}
						else
						{
							new_state.write_state = state;
							new_state.read_only_state.layout = new_state.write_state.layout;
						}
						it = layers_states.insert(it + 1, new_state);
					}

					if (range_max_layer < layers_end)
					{
						DoubleResourceState2 tmp_state{
							.write_state = it->write_state,
							.read_only_state = it->read_only_state,
						};
						
						if (state_is_readonly)
						{
							it->read_only_state = (state | it->read_only_state);
						}
						else
						{
							it->write_state = state;
							it->read_only_state = {};
							it->read_only_state.layout = it->write_state.layout;
						}
						
						it = layers_states.insert(it + 1, InternalStates::PosAndState{
							.pos = range_max_layer,
							.write_state = tmp_state.write_state,
							.read_only_state = tmp_state.read_only_state,
						});
					}
				}
			}

			assert(statesAreSorted(tid));

			bool reduce = false; // TODO
		}

	}






	Image::Image(CreateInfo const& ci) : 
		InstanceHolder<ImageInstance>(ci.app, ci.name, ci.hold_instance),
		_flags(ci.flags),
		_type(ci.type),
		_format(ci.format),
		_extent(ci.extent),
		_mips(ci.mips),
		_layers(ci.layers),
		_samples(ci.samples),
		_tiling(ci.tiling),
		_usage(ci.usage),
		_sharing_mode(ci.queues.size() <= 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT),
		_queues(ci.queues),
		_initial_layout(ci.initial_layout),
		_mem_usage(ci.mem_usage)
	{
		if(ci.create_on_construct && holdInstance().value())
			createInstance();
	}

	Image::Image(AssociateInfo const& assos):
		InstanceHolder<ImageInstance>(assos.instance->application(), assos.instance->name(), true)
	{
		associateImage(assos);
	}

	void Image::createInstance()
	{
		assert(!_inst);
		uint32_t n_queues = 0;
		uint32_t* p_queues = nullptr;
		if (_sharing_mode == VK_SHARING_MODE_CONCURRENT)
		{	
			n_queues = _queues.size();
			p_queues = _queues.data();
		}
		VkExtent3D extent = *_extent;
		const uint32_t mips = [&]() {
			uint32_t res = 1;
			const uint32_t desired = *_mips;
			if (desired == uint32_t(-1))
			{
				res = howManyMips(_type, extent);
				_inst_all_mips = true;
			}
			else
			{
				res = desired;
				_inst_all_mips = false;
			}
			return res;
		}();
		VkImageCreateInfo image_ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = _flags,
			.imageType = _type,
			.format = *_format,
			.extent = extent,
			.mipLevels = mips,
			.arrayLayers = *_layers,
			.samples = *_samples,
			.tiling = _tiling,
			.usage = _usage,
			.sharingMode = _sharing_mode,
			.queueFamilyIndexCount = n_queues,
			.pQueueFamilyIndices = p_queues,
			.initialLayout = _initial_layout,
		};

		VmaAllocationCreateInfo alloc{
			.usage = _mem_usage,
		};

		_inst = std::make_shared<ImageInstance>(ImageInstance::CI
		{
			.app = _app,
			.name = name(),
			.ci = image_ci,
			.aci = alloc,
		});
	}

	void Image::associateImage(AssociateInfo const& assos)
	{
		assert(_inst == nullptr);
		_app = assos.instance->application();
		_inst = assos.instance;
		if (!assos.instance->name().empty())
			_name = assos.instance->name().empty();

		_type = assos.instance->createInfo().imageType;
		_format = assos.format;
		_extent = assos.extent;
		_mips = assos.instance->createInfo().mipLevels;
		_layers = assos.instance->createInfo().arrayLayers; // TODO Check if the swapchain can support multi layered images (VR)
		_samples = assos.instance->createInfo().samples;
		_usage = assos.instance->createInfo().usage;
		_queues = std::vector<uint32_t>(assos.instance->createInfo().pQueueFamilyIndices, assos.instance->createInfo().pQueueFamilyIndices + assos.instance->createInfo().queueFamilyIndexCount);
		_sharing_mode = (_queues.size() <= 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		_mem_usage = assos.instance->AllocationInfo().usage;
		_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED; // In which layout are created swapchain images?+
	}


	bool Image::updateResource(UpdateContext & ctx)
	{
		if (ctx.updateTick() <= _latest_update_tick)
		{
			return _latest_update_res;
		}
		_latest_update_tick = ctx.updateTick();
		bool & res = _latest_update_res = false;

		if (checkHoldInstance())
		{
			using namespace vk_operators;
			if (_inst)
			{
				if (_inst->ownership())
				{
					const VkImageCreateInfo & inst_ci = _inst->createInfo();
					const VkExtent3D new_extent = *_extent;
					if (new_extent != inst_ci.extent)
					{
						res = true;
					}
					const uint32_t new_mips = *_mips;
					if (new_mips != inst_ci.mipLevels)
					{
						if (!(new_mips == uint32_t(-1) && _inst_all_mips))
						{
							res = true;
						}
					}
					const VkFormat new_format = *_format;
					if (new_format != inst_ci.format)
					{
						res = true;
					}
					const uint32_t new_layers = *_layers;
					if (new_layers != inst_ci.arrayLayers)
					{
						res = true;
					}
					const VkSampleCountFlagBits new_samples = *_samples;
					if (new_samples != inst_ci.samples)
					{
						res = true;
					}

					if (res)
					{
						destroyInstanceIFN();
					}
				}
			}

			if (!_inst)
			{
				createInstance();
				res = true;
			}
		}

		return res;
	}

	//VkImageSubresourceRange Image::defaultSubresourceRange()
	//{
	//	// Assume the dyn format keeps the same aspect,
	//	// else return a dynamic value
	//	VkImageAspectFlags aspect = getImageAspectFromFormat(_format.value());
	//	return VkImageSubresourceRange{
	//		.aspectMask = aspect, 
	//		.baseMipLevel = 0,
	//		.levelCount = _mips,
	//		.baseArrayLayer = 0,
	//		.layerCount = _layers.value(),
	//	};
	//}

	uint32_t Image::actualMipsCount()const
	{
		uint32_t res = _mips.valueOr(1);
		if (res == uint32_t(-1))
		{
			res = howManyMips(_type, *_extent);
		}
		return res;
	}

	Dyn<VkImageSubresourceRange> Image::fullSubresourceRange()
	{
		return [this]() {
			VkImageAspectFlags aspect = getImageAspectFromFormat(_format.value());		
			return VkImageSubresourceRange{
				.aspectMask = aspect,
				.baseMipLevel = 0,
				.levelCount = actualMipsCount(),
				.baseArrayLayer = 0,
				.layerCount = _layers.value(),
			};
		};
	}
}