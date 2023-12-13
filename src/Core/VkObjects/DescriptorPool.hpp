#pragma once

#include "DescriptorSetLayout.hpp"

namespace vkl
{
	class DescriptorPool : public VkObject
	{
	protected:

		// One xor the other
		std::shared_ptr<DescriptorSetLayoutInstance> _layout = nullptr;
		std::vector<VkDescriptorPoolSize> _pool_sizes = {};
		
		uint32_t _max_sets = 0;
		VkDescriptorPoolCreateFlags _flags = 0;
		VkDescriptorPool _handle = VK_NULL_HANDLE;

		void create();

		void destroy();

		void setVkName();
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<DescriptorSetLayoutInstance> layout = nullptr;
			VkDescriptorPoolCreateFlags flags = 0;
			uint32_t max_sets = 1;
		};
		using CI = CreateInfo;

		struct CreateInfoRaw
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkDescriptorPoolCreateFlags flags = 0;
			uint32_t max_sets = 1;
			std::vector<VkDescriptorPoolSize> sizes = {};
		};

		DescriptorPool(CreateInfo const& ci);

		DescriptorPool(CreateInfoRaw const& ci);

		virtual ~DescriptorPool() override;


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