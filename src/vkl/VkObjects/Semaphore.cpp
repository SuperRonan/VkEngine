#include <vkl/VkObjects/Semaphore.hpp>
#include <cassert>

namespace vkl
{
	Semaphore::Semaphore(VkApplication* app) :
		VkObject(app)
	{
		create();
	}

	Semaphore::~Semaphore()
	{
		if (_handle)
		{
			destroy();
		}
	}

	void Semaphore::create()
	{
		assert(_handle == VK_NULL_HANDLE);
		VkSemaphoreCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
		};
		VK_CHECK(vkCreateSemaphore(_app->device(), &ci, nullptr, &_handle), "Failed to create a semaphore.");
		if(!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT sem_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_SEMAPHORE,
				.objectHandle = (uint64_t)_handle,
				.pObjectName = name().data(),
			};
			_app->nameVkObjectIFP(sem_name);
		}
	}

	void Semaphore::destroy()
	{
		assert(_handle);
		vkDestroySemaphore(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}
}