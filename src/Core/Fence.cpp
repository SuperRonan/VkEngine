#include "Fence.hpp"
#include <cassert>

namespace vkl
{
	Fence::Fence(VkApplication* app, bool signaled) :
		VkObject(app)
	{
		create(signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0);
	}

	Fence::~Fence()
	{
		if(_handle)
			destroy();
	}

	void Fence::create(VkFenceCreateFlags flags)
	{
		assert(!_handle);
		VkFenceCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = flags,
		};
		VK_CHECK(vkCreateFence(_app->device(), &ci, nullptr, &_handle), "Failed to create a fence.");
	}

	void Fence::destroy()
	{
		assert(_handle);
		vkDestroyFence(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	void Fence::wait(uint64_t timeout)
	{
		assert(_handle);
		VK_CHECK(vkWaitForFences(_app->device(), 1, &_handle, true, timeout), "Failed to wait for a fence.");
	}

	void Fence::reset()
	{
		assert(_handle);
		VK_CHECK(vkResetFences(_app->device(), 1, &_handle), "Failed to reset a fence.");
	}

	void Fence::waitAndReset()
	{
		wait();
		reset();
	}
}