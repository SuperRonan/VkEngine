#include <vkl/VkObjects/Queue.hpp>

namespace vkl
{
	void Queue::get()
	{
		const VkDeviceQueueInfo2 info{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
			.pNext = nullptr,
			.flags = _flags,
			.queueFamilyIndex = _family_index,
			.queueIndex = _index,
		};
		vkGetDeviceQueue2(device(), &info, &_handle);
	}

	void Queue::setVkNameIFP()
	{
		application()->nameVkObjectIFP(VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(_handle), name());
	}

	Queue::Queue(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_flags(ci.flags),
		_family_index(ci.family_index),
		_index(ci.index),
		_properties(ci.properties),
		_priority(ci.priority)
	{
		get();
		setVkNameIFP();
	}

	Queue::~Queue()
	{

	}
}