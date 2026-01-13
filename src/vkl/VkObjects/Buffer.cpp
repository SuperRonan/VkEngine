#include <vkl/VkObjects/Buffer.hpp>
#include <cassert>

#include <vkl/GUI/Panel.hpp>
#include <vkl/GUI/InlinePanel.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/ImGuiDynamic.hpp>
#include <vkl/GUI/VulkanEnumWidgets.hpp>

#include <vkl/VkObjects/VulkanEnumMeta.hpp>


namespace vkl
{
	std::atomic<size_t> BufferInstance::_instance_counter = 0;

	BufferInstance::BufferInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_ci(ci.ci),
		_aci(ci.aci),
		_min_align(ci.min_align),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1)),
		_allocator(ci.allocator)
	{
		create();
	}

	BufferInstance::~BufferInstance()
	{
		if (!!_buffer)
		{
			destroy();
		}
	}

	void BufferInstance::create()
	{
		assert(!_buffer);
		bool can_device_address = application()->availableFeatures().features_12.bufferDeviceAddress;
		if (!can_device_address)
		{
			_ci.usage &= ~VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		}
		assert(_allocator);
		
		VK_CHECK(vmaCreateBufferWithAlignment(_allocator, &_ci, &_aci, _min_align, &_buffer, &_alloc, nullptr), "Failed to create a buffer.");
		
		InternalStates is;
		is.states.push_back(InternalStates::PosAndState{
			.pos = 0,
		});

		_states[0] = std::move(is);

		setVkName();


		if (_ci.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			VkBufferDeviceAddressInfo info{
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.pNext = nullptr,
				.buffer = _buffer,
			};
			_address = vkGetBufferDeviceAddress(device(), &info);
		}
	}

	void BufferInstance::destroy()
	{
		assert(!!_buffer);

		if (!!_data)
		{
			unMap();
		}

		callDestructionCallbacks();
		
		vmaDestroyBuffer(_allocator, _buffer, _alloc);
		_buffer = VK_NULL_HANDLE;
		_alloc = VMA_NULL;
	}

	void BufferInstance::setVkName()
	{
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT buffer_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_BUFFER,
				.objectHandle = (uint64_t)_buffer,
				.pObjectName = name().c_str(),
			};
			_app->nameVkObjectIFP(buffer_name);
		}
	}

	bool BufferInstance::statesAreSorted(size_t tid) const
	{
		assert(_states.contains(tid));
		const auto & states = _states.at(tid).states;
		for (size_t i = 1; i < states.size(); ++i)
		{
			if (states[i - 1].pos >= states[i].pos)
			{
				return false;
			}
		}
		return true;
	}


	void * BufferInstance::map()
	{
		assert(_buffer != VK_NULL_HANDLE);
		vmaMapMemory(_allocator, _alloc, &_data);
		return _data;
	}

	void BufferInstance::unMap()
	{
		assert(_buffer != VK_NULL_HANDLE);
		vmaUnmapMemory(_allocator, _alloc);
		_data = nullptr;
	}

	void BufferInstance::flush()
	{
		assert(_buffer != VK_NULL_HANDLE);
		vmaFlushAllocation(_allocator, _alloc, 0, _ci.size);
	}

	DoubleDoubleResourceState2 BufferInstance::getState(size_t tid, Range r) const
	{
		assert(statesAreSorted(tid));

		if (r.len == 0 || r.len == VK_WHOLE_SIZE)
		{
			r.len = (_ci.size - r.begin);
		}
		const size_t range_end = r.begin + r.len;
		assert(_states.contains(tid));
		const InternalStates & is = _states.at(tid);
		assert(!is.states.empty());
		assert(is.states[0].pos == 0);

		DoubleDoubleResourceState2  res;
		res.multiplicative.write_state = ResourceState2::Full();
		res.multiplicative.read_only_state = ResourceState2::Full();

		for (size_t i = 0; i < is.states.size(); ++i)
		{
			const size_t range_i_end = [&]()
			{
				if(i == is.states.size() - 1)	return _ci.size;
				else							return is.states[i+1].pos;
			}();

			if (r.begin >= range_i_end) // Not yet there
			{
				continue;
			}
			if (is.states[i].pos >= range_end) // Done
			{
				break;
			}

			const ResourceState2 & rw_state = is.states[i].write_state;
			const ResourceState2 & ro_state = is.states[i].read_only_state;

			// The sub range intersects with the requested range
			res.additive.write_state |= rw_state;
			res.additive.read_only_state |= ro_state;

			res.multiplicative.write_state &= rw_state;
			res.multiplicative.read_only_state &= ro_state;
		}

		return res;
	}

	void BufferInstance::setState(size_t tid, Range r, ResourceState2 const& state)
	{
		const bool state_is_readonly = accessIsReadonly2(state.access);

		if (r.len == 0 || r.len == VK_WHOLE_SIZE)
		{
			r.len = (_ci.size - r.begin);
		}
		const size_t range_end = r.begin + r.len;
		assert(_states.contains(tid));
		InternalStates& is = _states[tid];

		for (auto it = is.states.begin(); it != is.states.end(); ++it)
		{
			assert(is.states[0].pos == 0);
			const size_t range_i_end = [&]()
			{
				if ((it+1) == is.states.end())	return _ci.size;
				else							return (it+1)->pos;
			}();

			if (r.begin >= range_i_end) // Not yet there
			{
				continue;
			}
			if (it->pos >= range_end) // Done
			{
				break;
			}

			// The sub range intersects with the requested range

			if (r.begin <= it->pos && range_end >= range_i_end) // range_i is a subset of r
			{
				if (state_is_readonly)
				{
					it->read_only_state |= state;
				}
				else
				{
					it->write_state = state;
					it->read_only_state = {};
				}
			}
			else
			{
				if (r.begin > it->pos)
				{
					InternalStates::PosAndState new_state {
						.pos = r.begin,
					};
					if (state_is_readonly)
					{
						new_state.read_only_state = it->read_only_state | state;
						new_state.write_state = it->write_state;
					}
					else
					{
						new_state.write_state = state;
					}
					it = is.states.insert(it + 1, new_state);
				}

				if (range_end < range_i_end)
				{
					DoubleResourceState2 tmp_state {.write_state = it->write_state, .read_only_state = it->read_only_state};
					
					if (state_is_readonly)
					{
						it->read_only_state |= state;
					}
					else
					{
						it->write_state = state;
						it->read_only_state = {};
					}

					it = is.states.insert(it + 1, InternalStates::PosAndState {
						.pos = range_end,
						.write_state = tmp_state.write_state,
						.read_only_state = tmp_state.read_only_state,
					});
				}
			}
		}

		assert(statesAreSorted(tid));

		// I don't think it is always necessary, maybe do it periodically
		bool reduce = false;

		if (reduce)
		{
			for (auto it = is.states.begin(); (it+1) != is.states.end(); ++it)
			{
				const auto next = it + 1;
				if ((it->write_state == next->write_state) && (it->read_only_state == next->read_only_state))
				{
					it = is.states.erase(next);
				}
			}
			assert(statesAreSorted(tid));
		}

	}

	class BufferInstanceInspector : public GUI::Panel
	{
	protected:
		std::shared_ptr<BufferInstance> _target;

	
	public:
	
		BufferInstanceInspector(std::shared_ptr<BufferInstance> const& target):
			Panel(target->application(), std::format("{} - Instance##{}", target->name(), reinterpret_cast<uintptr_t>(target.get()))),
			_target(target)
		{
			
		}

		virtual void declareInline(GUI::Context& ctx) override
		{
			auto const& ci = _target->_ci;
			
			ImGui::LabelHexValue("Minimum Align", _target->_min_align);

			GUI::InspectVkBitField<VkBufferCreateFlagBits>(ctx, "Creation Flags", _target->createInfo().flags);
			ImGui::LabelValue("Size", _target->_ci.size);
			GUI::InspectVkBitField<VkBufferUsageFlagBits>(ctx, "Usage", _target->createInfo().usage);
			GUI::InspectVkEnum(ctx, "Sharing Mode", _target->_ci.sharingMode);
			ImGui::LabelValue("Queue family index count", _target->_ci.queueFamilyIndexCount); // TODO proper span inspector
			// TODO inspect list of queues

			ImGui::LabelHexValue("Handle", reinterpret_cast<uint64_t>(_target->handle()));
			ImGui::LabelHexValue("Unique Buffer Id", _target->_unique_id);
			ImGui::LabelHexValue("Address", _target->_address, true);

			ImGui::SeparatorText("Allocation");
			const auto& aci = _target->allocationCreateInfo();
			GUI::InspectVkBitField<VmaAllocationCreateFlagBits>(ctx, "Flags##Allocation", aci.flags);
			GUI::InspectVkEnum(ctx, "Memory Usage", aci.usage);
			GUI::InspectVkBitField<VkMemoryPropertyFlagBits>(ctx, "Required Flags", aci.requiredFlags);
			GUI::InspectVkBitField<VkMemoryPropertyFlagBits>(ctx, "Preferred Flags", aci.preferredFlags);
			ImGui::LabelHexValue("Memory type bits", aci.memoryTypeBits); // TODO proper inspector
			ImGui::LabelHexValue("Pool", reinterpret_cast<uintptr_t>(aci.pool));
			ImGui::LabelValue("Priority", aci.priority);

			// TODO inspect allocation

			// TODO inspect states
		}

	};

	class BufferInspector : public GUI::Panel
	{
	protected:
		
		std::shared_ptr<Buffer> _target;
		GUI::IndirectInlinePanel _instance_panel;

	public:
		BufferInspector(std::shared_ptr<Buffer> const& target):
			Panel(target->application(), std::format("{} - Descriptor##{}", target->name(), reinterpret_cast<uintptr_t>(target.get()))),
			_target(target)
		{
			_instance_panel = GUI::IndirectInlinePanel::MakeInstanceIndirectPanelFromDesc(_target);
			_instance_panel.type = GUI::InlinePanel::Type::Child;
		}

		virtual void declareInline(GUI::Context& ctx) override
		{
			ImGui::LabelText2("Name", _target->name().c_str());
			GUI::DeclareDynamic("Size", _target->_size, [](const char* label, VkDeviceSize& sz){ImGui::LabelValue(label, sz); return false; });
			ImGui::LabelHexValue("Minimum Align", _target->_min_align);
			GUI::InspectVkBitField<VkBufferUsageFlagBits>(ctx, "Usage", &_target->_usage);
			ImGui::LabelValue("Queues family index count", uint32_t(_target->_queues.size())); // TODO inspect queues
			GUI::InspectVkEnum(ctx, "Sharing Mode", _target->_sharing_mode);
			GUI::InspectVkEnum(ctx, "Memory Usage", _target->_mem_usage);
			ImGui::LabelHexValue("Allocator", reinterpret_cast<uintptr_t>(_target->_allocator));

			_instance_panel.invalid_panel = !_target->instance();
			_instance_panel.id = reinterpret_cast<GUI::Panel::Id>(_target->instance().get());
			_instance_panel.declareInline(ctx);
		}
	};

	std::shared_ptr<GUI::Panel> BufferInstance::makeInspector(std::shared_ptr<BufferInstance> const& shared_this, GUI::Context& ctx)
	{
		return std::make_shared<BufferInstanceInspector>(shared_this);
	}

	std::shared_ptr<GUI::Panel> Buffer::makeInspector(std::shared_ptr<Buffer> const& shared_this, GUI::Context& ctx)
	{
		return std::make_shared<BufferInspector>(shared_this);
	}

	Buffer::Buffer(CreateInfo const& ci) :
		InstanceHolder<BufferInstance>(ci.app, ci.name, ci.hold_instance),
		_size(ci.size),
		_min_align(ci.min_align),
		_usage(ci.usage),
		_queues(std::filterRedundantValues(ci.queues)),
		_sharing_mode(_queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE),
		_mem_usage(ci.mem_usage),
		_allocator(ci.allocator ? ci.allocator : _app->allocator())
	{
		if (ci.create_on_construct && holdInstance().value())
		{
			createInstance();
		}
	}

	Buffer::~Buffer()
	{
		
	}

	void Buffer::createInstance()
	{
		BufferInstance::CreateInfo ci{
			.app = application(),
			.name = name(),
			.ci = VkBufferCreateInfo {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = *_size,
				.usage = _usage,
				.sharingMode = _sharing_mode,
				.queueFamilyIndexCount = (uint32_t)_queues.size(),
				.pQueueFamilyIndices = _queues.data(),
			},
			.aci = VmaAllocationCreateInfo{
				.usage = _mem_usage,
			},
			.min_align = _min_align,
			.allocator = _allocator,
		};
		
		_inst = std::make_shared<BufferInstance>(ci);
	}

	void Buffer::updateResourcesInline(UpdateContext & ctx, UpdateResourcesResult& res)
	{
		if (_inst)
		{
			if (_inst->createInfo().size != *_size)
			{
				res.invalidated = true;
			}
			
			if (res.invalidated)
			{
				destroyInstanceIFN();
			}
		}

		if (!_inst)
		{
			res.created = true;
			createInstance();
		}
	}
}