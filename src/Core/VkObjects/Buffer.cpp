#include "Buffer.hpp"
#include <cassert>

namespace vkl
{
	std::atomic<size_t> BufferInstance::_instance_counter = 0;


	BufferInstance::BufferInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_ci(ci.ci),
		_aci(ci.aci),
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
		VK_CHECK(vmaCreateBuffer(_allocator, &_ci, &_aci, &_buffer, &_alloc, nullptr), "Failed to create a buffer.");

		InternalStates is;
		is.states.push_back(InternalStates::PosAndState{
			.pos = 0,
		});

		_states[0] = std::move(is);

		setVkName();
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


	void BufferInstance::map()
	{
		assert(_buffer != VK_NULL_HANDLE);
		vmaMapMemory(_allocator, _alloc, &_data);
	}

	void BufferInstance::unMap()
	{
		assert(_buffer != VK_NULL_HANDLE);
		vmaUnmapMemory(_allocator, _alloc);
		_data = nullptr;
	}

	DoubleResourceState2 BufferInstance::getState(size_t tid, Range r) const
	{
		assert(statesAreSorted(tid));

		if (r.len == 0 || r.len == VK_WHOLE_SIZE)
		{
			r.len = (_ci.size - r.begin);
		}
		const size_t range_end = r.begin + r.len;
		assert(_states.contains(tid));
		const InternalStates & is = _states.at(tid);
		assert(is.states[0].pos == 0);

		DoubleResourceState2  res;

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
			res.write_state |= rw_state;
			res.read_only_state |= ro_state;
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
		}

		assert(statesAreSorted(tid));
	}



	Buffer::Buffer(CreateInfo const& ci) :
		InstanceHolder<BufferInstance>(ci.app, ci.name),
		_size(ci.size),
		_usage(ci.usage),
		_queues(std::filterRedundantValues(ci.queues)),
		_sharing_mode(_queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE),
		_mem_usage(ci.mem_usage),
		_allocator(ci.allocator ? ci.allocator : _app->allocator())
	{
		if (ci.create_on_construct)
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
			.allocator = _allocator,
		};
		
		_inst = std::make_shared<BufferInstance>(ci);
	}

	void Buffer::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	bool Buffer::updateResource(UpdateContext & ctx)
	{
		bool res = false;
		if (_inst)
		{
			if (_inst->createInfo().size != *_size)
			{
				destroyInstance();
			}
		}

		if (!_inst)
		{
			createInstance();
			res = true;
		}

		return res;
	}

	

	//StagingPool::StagingBuffer * Buffer::copyToStaging(void* data, size_t size)
	//{
	//	if (size == 0)	size = _size;
	//	StagingPool::StagingBuffer* sb = _app->stagingPool().getStagingBuffer(size);

	//	std::memcpy(sb->data, data, size);

	//	return sb;
	//}

	//void Buffer::recordCopyStagingToBuffer(VkCommandBuffer cmd, StagingPool::StagingBuffer* sb)
	//{
	//	VkBufferCopy copy{
	//		.srcOffset = 0,
	//		.dstOffset = 0,
	//		.size = std::min(_size, sb->size),
	//	};

	//	vkCmdCopyBuffer(cmd, sb->buffer, _buffer, 1, &copy);
	//}
}