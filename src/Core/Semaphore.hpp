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

		template <typename StringLike = std::string>
		Semaphore(VkApplication* app, StringLike&& name) :
			VkObject(app, std::forward<StringLike>(name))
		{
			create();
		}

		Semaphore(Semaphore const&) = delete;

		constexpr Semaphore(Semaphore&& other) noexcept :
			VkObject(std::move(other)),
			_handle(other._handle)
		{
			other._handle = VK_NULL_HANDLE;
		}

		Semaphore& operator=(Semaphore const&) = delete;

		constexpr Semaphore& operator=(Semaphore&& other) noexcept
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