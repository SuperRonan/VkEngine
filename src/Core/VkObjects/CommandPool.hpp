#pragma once

#include <Core/App/VkApplication.hpp>

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
