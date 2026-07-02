#include <vkl/VkObjects/Image.hpp>
#include <vkl/VkObjects/ImageView.hpp>

#include <vkl/GUI/DescriptorInstancePanel.hpp>
#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/VulkanEnumWidgets.hpp>
#include <vkl/GUI/ImGuiDynamic.hpp>
#include <vkl/GUI/ImageVisualizer.hpp>

namespace vkl
{
	std::atomic<size_t> ImageInstance::_instance_counter = 0;

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
		assert(handle() == VK_NULL_HANDLE);

		VK_CHECK(vmaCreateImage(_app->allocator(), &_ci, &_vma_ci, &handle(), &_alloc, nullptr), "Failed to create an image.");

		registerName();
	}

	void ImageInstance::destroy()
	{
		assert(handle() != VK_NULL_HANDLE);

		callDestructionCallbacks();

		if (ownership())
		{
			vmaDestroyImage(_app->allocator(), handle(), _alloc);
		}

		_handle = VK_NULL_HANDLE;
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

	void ImageInstance::setQueues()
	{
		if (_ci.queueFamilyIndexCount == 0)
		{
			_ci.pQueueFamilyIndices = nullptr;
		}
		std::span<const uint32_t> queues(_ci.pQueueFamilyIndices, _ci.queueFamilyIndexCount);
		_queues.assign(queues.begin(), queues.end());
		_ci.pQueueFamilyIndices = _queues.data();
	}

	void ImageInstance::setMipsCount()
	{
		if (_ci.mipLevels == Image::ALL_MIPS)
		{
			_remaining_mips = true;
			_ci.mipLevels = Image::HowManyMips(_ci.imageType, _ci.extent);
		}
		auto info = imageFormatInfo2();
		VkImageFormatProperties2 props{
			.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
			.pNext = nullptr,
		};
		vkGetPhysicalDeviceImageFormatProperties2(application()->physicalDevice(), &info, &props);
		// Not 100% necessary, since the API max mip level given here depends on the max extent, which we do not check.
		_ci.mipLevels = std::min(_ci.mipLevels, props.imageFormatProperties.maxMipLevels);
	}

	ImageInstance::ImageInstance(CreateInfo const& ci) :
		Parent(ci.app, ci.name, ci.tick),
		_ci(ci.ci),
		_vma_ci(ci.aci),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{
		setMipsCount();
		setQueues();
		create();
		setInitialState(0);
	}

	ImageInstance::ImageInstance(AssociateInfo const& ai) :
		Parent(ai.app, ai.name, ai.tick),
		_ci(ai.ci),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{
		assert(_ci.mipLevels != ALL_MIPS);
		setQueues();
		_handle = ai.image;
		registerName();
		setInitialState(0);
	}

	ImageInstance::~ImageInstance()
	{
		if (!!_handle)
		{
			destroy();
		}
	}

	void ImageInstance::fillState(size_t tid, Range const& _range, MyVector<StateInRange> & res) const
	{
		assert(statesAreSorted(tid));
		Range range = finiteRange(_range);
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

	void ImageInstance::setState(size_t tid, Range const& _range, ResourceState2 const& state)
	{
		const bool state_is_readonly = accessIsReadonly2(state.access);
		Range range = finiteRange(_range);
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
		Parent(ci.app, ci.name, ci.hold_instance),
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

	Image::Image(std::shared_ptr<ImageInstance> const& inst) :
		Parent(inst)
	{
		const VkImageCreateInfo& ci = instance()->createInfo();
		_flags = ci.flags;
		_type = ci.imageType;
		_format = ci.format;
		_extent = ci.extent;
		_mips = ci.mipLevels;
		_layers = ci.arrayLayers;
		_samples = ci.samples;
		_tiling = ci.tiling;
		_usage = ci.usage;
		_queues = instance()->queues();
		_sharing_mode = ci.sharingMode;
		_initial_layout = ci.initialLayout;
		_mem_usage = instance()->allocationInfo().usage;
	}

	void Image::destroyInstanceIFN()
	{
		if (_instance && !instance()->ownership())
		{
			
		}
		else
		{
			InstanceHolder<ImageInstance>::destroyInstanceIFN();
		}
	}

	void Image::createInstance(size_t tick)
	{
		assert(!_instance);
		uint32_t n_queues = 0;
		uint32_t* p_queues = nullptr;
		if (_sharing_mode == VK_SHARING_MODE_CONCURRENT)
		{	
			n_queues = _queues.size();
			p_queues = _queues.data();
		}
		VkExtent3D extent = *_extent;
		uint32_t mips = *_mips;
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

		_instance = std::make_shared<ImageInstance>(ImageInstance::CI
		{
			.app = _app,
			.name = name(),
			.tick = tick,
			.ci = image_ci,
			.aci = alloc,
		});
	}


	void Image::updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res)
	{
		using namespace vk_operators;
		if (_instance)
		{
			if (instance()->ownership())
			{
				res.invalidated = [&]()
				{
					const VkImageCreateInfo & inst_ci = instance()->createInfo();
					const VkExtent3D new_extent = *_extent;
					if (new_extent != inst_ci.extent)
					{
						return true;
					}
					const uint32_t new_mips = *_mips;
					if (new_mips != instance()->mipLevels())
					{
						return true;
					}
					const VkFormat new_format = *_format;
					if (new_format != inst_ci.format)
					{
						return true;
					}
					const uint32_t new_layers = *_layers;
					if (new_layers != inst_ci.arrayLayers)
					{
						return true;
					}
					const VkSampleCountFlagBits new_samples = *_samples;
					if (new_samples != inst_ci.samples)
					{
						return true;
					}
					return false;
				}();
				if (res.invalidated)
				{
					destroyInstanceIFN();
				}
			}
		}

		if (!_instance)
		{
			res.created = true;
			createInstance(ctx.updateTick());
		}
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
		if (res == ALL_MIPS)
		{
			res = HowManyMips(_type, *_extent);
		}
		return res;
	}

	VkImageSubresourceRange Image::finiteRange(VkImageSubresourceRange const& range) const
	{
		// "Optimized version" that evaluates dynamic values only if needed
		VkImageSubresourceRange res = range;
		VkExtent3D extent = {};
		if (res.levelCount == ALL_MIPS)
		{
			extent = _extent.value();
			uint32_t mips = _mips.value();
			if (mips == ALL_MIPS)
			{
				mips = HowManyMips(_type, extent);
			}
			res.levelCount = mips - res.baseMipLevel;
		}
		if (res.layerCount == ALL_LAYERS)
		{
			uint32_t layers;
			// See comments in ImageInstance::finiteRange()
			if (_type == VK_IMAGE_TYPE_3D)
			{
				if (extent.depth == 0)
				{
					extent = _extent.value();
				}
				layers = extent.depth;
			}
			else
			{
				layers = _layers.value();
			}
			res.layerCount = layers - res.baseArrayLayer;
		}
		return res;
	}
	
	namespace GUI
	{
		class ImageInstanceInspector : public InstanceInspector<ImageInstance>
		{
			using Parent = InstanceInspector<ImageInstance>;
		protected:

			ImageVisualizer _visualizer;

		public:

			ImageInstanceInspector(std::shared_ptr<ImageInstance> const& target, Context& ctx):
				Parent(target),
				_visualizer(ImageVisualizer::CI{
					.ctx = &ctx,
					.label = "Visualizer",
				})
			{

			}

			virtual void declareInline(Context& ctx) override
			{
				const auto& ci = target()->createInfo();
				const auto& aci = target()->allocationInfo();
				InspectVkBitField<VkImageCreateFlagBits>(ctx, "Creation Flags", ci.flags);
				InspectVkEnum(ctx, "Type", ci.imageType);
				InspectVkEnum(ctx, "Format", ci.format);
				ImGui::InputScalarN("Extent", ImGuiDataType_U32, const_cast<uint32_t*>(&ci.extent.width), 3, nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly);
				ImGui::InputScalarN("Mips / Layers", ImGuiDataType_U32, const_cast<uint32_t*>(&ci.mipLevels), 2 , nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly);
				uint32_t sample_count = std::countr_zero(uint32_t(ci.samples));
				ImGui::LabelValue("Samples", sample_count);
				InspectVkEnum(ctx, "Tiling", ci.tiling);
				InspectVkBitField<VkImageUsageFlagBits>(ctx, "Usage", ci.usage);
				InspectVkEnum(ctx, "Sharing Mode", ci.sharingMode);
				ImGui::LabelValue("Queue family index count", ci.queueFamilyIndexCount); // TODO proper span inspector
				InspectVkEnum(ctx, "Initial Layout", ci.initialLayout);

				ImGui::SeparatorText("Allocation");
				InspectVkBitField<VmaAllocationCreateFlagBits>(ctx, "Flags##Allocation", aci.flags);
				InspectVkEnum(ctx, "Memory Usage", aci.usage);
				InspectVkBitField<VkMemoryPropertyFlagBits>(ctx, "Required Flags", aci.requiredFlags);
				InspectVkBitField<VkMemoryPropertyFlagBits>(ctx, "Preferred Flags", aci.preferredFlags);
				ImGui::LabelHexValue("Memory type bits", aci.memoryTypeBits); // TODO proper inspector
				ImGui::LabelHexValue("Pool", reinterpret_cast<uintptr_t>(aci.pool));
				ImGui::LabelValue("Priority", aci.priority);

				SectionBox visu_section{};
				visu_section.child_flags |= ImGuiChildFlags_AutoResizeY;
				visu_section.label = _visualizer.label().data();
				if (visu_section.begin(ctx))
				{
					_visualizer.setSource(targetPtr());
					_visualizer.declareInline(ctx);
				}
				visu_section.end(ctx);
			}
		};

		class ImageInspector : public DescriptorInspector<Image>
		{
			using Parent = DescriptorInspector<Image>;
		protected:

			ImageVisualizer _visualizer;

		public:

			ImageInspector(std::shared_ptr<Image> const& target, Context& ctx) :
				Parent(target),
				_visualizer(ImageVisualizer::CI{
					.ctx = &ctx,
					.label = "Visualizer",
				})
			{

			}

			virtual void declareInline(Context& ctx) override
			{
				InspectVkBitField<VkImageCreateFlagBits>(ctx, "Creation Flags", target()->_flags);
				InspectVkEnum(ctx, "Type", target()->_type);
				DeclareDynamic("Format", target()->_format, [&](const char* label, VkFormat& format){InspectVkEnum(label, format); return false; });
				// TODO

				Parent::declareInstance(ctx);

				SectionBox visu_section{};
				visu_section.child_flags |= ImGuiChildFlags_AutoResizeY;
				visu_section.label = _visualizer.label().data();
				if (visu_section.begin(ctx))
				{
					_visualizer.setSource(targetPtr());
					_visualizer.declareInline(ctx);
				}
				visu_section.end(ctx);
			}
		};
	}

	std::shared_ptr<GUI::Panel> ImageInstance::makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx)
	{
		assert(shared_this.get() == this);
		return GUI::MakeInspectorFromTarget(ctx, std::static_pointer_cast<ImageInstance>(shared_this));
	}

	std::shared_ptr<GUI::Panel> Image::makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx)
	{
		assert(shared_this.get() == this);
		return GUI::MakeInspectorFromTarget<Image>(ctx, std::static_pointer_cast<Image>(shared_this));
	}
}