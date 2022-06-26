#include "Semaphore.hpp"
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
	}

	void Semaphore::destroy()
	{
		assert(_handle);
		vkDestroySemaphore(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}
}