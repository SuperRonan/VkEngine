#pragma once

#include "VkApplication.hpp"
#include "CommandPool.hpp"
#include <memory>

namespace vkl
{
	class CommandBuffer : public VkObject
	{
	protected:

		std::shared_ptr<CommandPool> _pool = nullptr;
		VkCommandBufferLevel _level = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
		VkCommandBuffer _handle = VK_NULL_HANDLE;

	public:

		constexpr CommandBuffer(VkApplication * app = nullptr) noexcept:
			VkObject(app)
		{}

		CommandBuffer(VkApplication* app, VkCommandBuffer handle, std::shared_ptr<CommandPool> pool, VkCommandBufferLevel level);

		CommandBuffer(std::shared_ptr<CommandPool> pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		CommandBuffer(CommandBuffer const&) = delete;
		CommandBuffer& operator=(CommandBuffer const&) = delete;

		CommandBuffer(CommandBuffer&& other);

		CommandBuffer& operator=(CommandBuffer&& other);

		~CommandBuffer();

		void allocate(VkCommandBufferAllocateInfo const& alloc);

		void destroy();

		void begin(VkCommandBufferUsageFlags usage = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		void end();

		void reset(VkCommandBufferResetFlags flag = 0);

		void submit(VkQueue queue);

		void submitAndWait(VkQueue queue);

		constexpr operator VkCommandBuffer()const
		{
			return _handle;
		}

		constexpr auto handle()const
		{
			return _handle;
		}

		constexpr const auto& pool()const
		{
			return _pool;
		}

	};
}