#pragma once

#include "VkApplication.hpp"

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

		Fence(Fence const&) = delete;

		constexpr Fence(Fence&& other) noexcept :
			VkObject(std::move(other)),
			_handle(other._handle)
		{
			other._handle = VK_NULL_HANDLE;
		}

		~Fence();

		Fence& operator=(Fence const&) = delete;

		Fence& operator=(Fence&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_handle, other._handle);
			return *this;
		}

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

		void reset();

		void waitAndReset();
	};
}