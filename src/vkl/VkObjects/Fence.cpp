#include <vkl/VkObjects/Fence.hpp>
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
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT fence_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_FENCE,
				.objectHandle = (uint64_t)_handle,
				.pObjectName = name().data(),
			};
			_app->nameVkObjectIFP(fence_name);
		}
	}

	void Fence::destroy()
	{
		assert(_handle);
		vkDestroyFence(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	VkResult Fence::wait(uint64_t timeout)
	{
		assert(_handle);
		return vkWaitForFences(_app->device(), 1, &_handle, true, timeout);
	}

	void Fence::reset()
	{
		assert(_handle);
		VK_CHECK(vkResetFences(_app->device(), 1, &_handle), "Failed to reset a fence.");
	}

	VkResult Fence::waitAndReset(uint64_t timeout)
	{
		VkResult res = wait(timeout);
		if (res == VK_SUCCESS)
		{
			reset();
		}
		return res;
	}
}