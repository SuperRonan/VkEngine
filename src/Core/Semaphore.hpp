#pragma once

#include "VkApplication.hpp"

namespace vkl
{
	class Semaphore : public VkObject
	{
	protected:

		VkSemaphore _handle = VK_NULL_HANDLE;

	public:

		constexpr Semaphore() noexcept = default;

		constexpr Semaphore(VkApplication* app, VkSemaphore handle) noexcept :
			VkObject(app),
			_handle(handle)
		{}

		Semaphore(VkApplication* app);

		Semaphore(Semaphore const&) = delete;

		Semaphore(Semaphore&& other) noexcept :
			VkObject(std::move(other)),
			_handle(other._handle)
		{
			other._handle = VK_NULL_HANDLE;
		}

		Semaphore& operator=(Semaphore const&) = delete;

		Semaphore& operator=(Semaphore&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_handle, other._handle);
			return *this;
		}

		~Semaphore();

		void create();

		void destroy();

		constexpr operator VkSemaphore() const
		{
			return _handle;
		}

		constexpr VkSemaphore handle()const
		{
			return _handle;
		}
	};
}