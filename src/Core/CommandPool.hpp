#pragma once

#include "VkApplication.hpp"

namespace vkl
{
	class CommandPool : public VkObject
	{
	protected:

		uint32_t _queue_family_index = 0;
		VkCommandPoolCreateFlags _flags = 0;
		VkCommandPool _handle = VK_NULL_HANDLE;

	public:

		constexpr CommandPool(VkApplication * app = nullptr) noexcept:
			VkObject(app)
		{}

		constexpr CommandPool(VkApplication* app, VkCommandPool handle, uint32_t qfi, VkCommandPoolCreateFlags flags = 0) noexcept :
			VkObject(app),
			_queue_family_index(qfi),
			_flags(flags),
			_handle(handle)
		{}

		CommandPool(VkApplication* app, uint32_t index, VkCommandPoolCreateFlags flags = 0);

		CommandPool(CommandPool const&) = delete;
		CommandPool& operator=(CommandPool const& other) = delete;

		constexpr CommandPool(CommandPool&& other) noexcept :
			VkObject(std::move(other)),
			_queue_family_index(other._queue_family_index),
			_flags(other._flags),
			_handle(other._handle)
		{
			other._handle = VK_NULL_HANDLE;
		}

		constexpr CommandPool& operator=(CommandPool&& other)noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_queue_family_index, other._queue_family_index);
			std::swap(_flags, other._flags);
			std::swap(_handle, other._handle);
			return *this;
		}

		~CommandPool();

		void create(VkCommandPoolCreateInfo const& ci);

		void destroy();

		constexpr operator VkCommandPool()const
		{
			return _handle;
		}

		constexpr auto handle()const
		{
			return _handle;
		}

		constexpr uint32_t index()const
		{
			return _queue_family_index;
		}

		constexpr VkCommandPoolCreateFlags flags()const
		{
			return _flags;
		}
		

	};

} // namespace vkl
