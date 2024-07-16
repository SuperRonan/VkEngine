#pragma once

#include <vkl/App/VkApplication.hpp>
#include <mutex>

namespace vkl
{
	class CommandPool : public VkObject
	{
	protected:

		uint32_t _queue_family = 0;
		VkCommandPoolCreateFlags _flags = 0;
		VkCommandPool _handle = VK_NULL_HANDLE;

		std::mutex _mutex;

		void setVkNameIFP();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			uint32_t queue_family = 0;
			VkCommandPoolCreateFlags flags = 0;
		};
		using CI = CreateInfo;

		CommandPool(CreateInfo const& ci);

		virtual ~CommandPool() override;

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

		constexpr uint32_t queueFamily()const
		{
			return _queue_family;
		}

		constexpr VkCommandPoolCreateFlags flags()const
		{
			return _flags;
		}
		
		std::mutex& mutex()
		{
			return _mutex;
		}
	};

} // namespace vkl
