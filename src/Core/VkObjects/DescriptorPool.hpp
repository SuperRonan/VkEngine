#pragma once

#include "DescriptorSetLayout.hpp"

namespace vkl
{
	class DescriptorPool : public VkObject
	{
	protected:

		std::shared_ptr<DescriptorSetLayout> _layout = nullptr;
		std::vector<VkDescriptorPoolSize> _pool_sizes = {};
		uint32_t _max_sets = 0;
		VkDescriptorPoolCreateFlags _flags = 0;
		VkDescriptorPool _handle = VK_NULL_HANDLE;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<DescriptorSetLayout> layout = nullptr;
			VkDescriptorPoolCreateFlags flags = 0;
			uint32_t max_sets = 1;
		};
		using CI = CreateInfo;

		DescriptorPool(VkApplication* app, std::string const& name = "") :
			VkObject(app, name)
		{}

		DescriptorPool(CreateInfo const& ci);

		virtual ~DescriptorPool() override;

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

		constexpr VkDescriptorPoolCreateFlags flags()const
		{
			return _flags;
		}
	};
}