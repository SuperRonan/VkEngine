#pragma once

#include "DescriptorSetLayout.hpp"

namespace vkl
{
	class DescriptorPool : public VkObject
	{
	protected:

		std::shared_ptr<DescriptorSetLayout> _layout = nullptr;
		std::vector<VkDescriptorPoolSize> _pool_sizes = {};
		uint32_t _max_sets;
		VkDescriptorPool _handle = VK_NULL_HANDLE;

	public:

		constexpr DescriptorPool(VkApplication* app = nullptr, VkDescriptorPool handle = VK_NULL_HANDLE) noexcept :
			VkObject(app),
			_handle(handle)
		{}

		DescriptorPool(std::shared_ptr<DescriptorSetLayout> layout, uint32_t max_sets = 1);

		DescriptorPool(DescriptorPool const&) = delete;

		DescriptorPool(DescriptorPool&& other) noexcept;

		DescriptorPool& operator=(DescriptorPool const&) = delete;

		DescriptorPool& operator=(DescriptorPool&&) noexcept;

		~DescriptorPool();

		void create(VkDescriptorPoolCreateInfo const& ci);

		void destroy();

		constexpr operator VkDescriptorPool()const
		{
			return _handle;
		}

		constexpr VkDescriptorPool handle()const
		{
			return _handle;
		}

		constexpr const auto& layout()const
		{
			return _layout;
		}

		constexpr auto& layout()
		{
			return _layout;
		}

	};
}