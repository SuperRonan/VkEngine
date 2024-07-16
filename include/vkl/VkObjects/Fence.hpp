#pragma once

#include <vkl/App/VkApplication.hpp>

namespace vkl
{
	class Fence : public VkObject
	{
	protected:

		VkFence _handle = VK_NULL_HANDLE;

	public:

		constexpr Fence() noexcept = default;

		constexpr Fence(VkApplication * app, VkFence fence) noexcept : 
			VkObject(app),
			_handle(fence)
		{}

		Fence(VkApplication* app, bool signaled = false);

		template <typename StringLike = std::string>
		Fence(VkApplication* app, StringLike&& name, bool signaled = false):
			VkObject(app, std::forward<StringLike>(name))
		{
			create(signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0);
		}

		virtual ~Fence() override;

		

		void create(VkFenceCreateFlags flags = 0);

		void destroy();

		constexpr operator VkFence()const
		{
			return _handle;
		}

		constexpr auto handle()const
		{
			return _handle;
		}

		void wait(uint64_t timeout = UINT64_MAX);

		VkResult getStatus()const
		{
			return vkGetFenceStatus(device(), _handle);
		}

		void reset();

		void waitAndReset();
	};
}