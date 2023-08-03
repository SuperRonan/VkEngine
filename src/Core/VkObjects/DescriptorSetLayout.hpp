#pragma once

#include <Core/App/VkApplication.hpp>

namespace vkl
{
	class DescriptorSetLayout : public VkObject
	{
	public:

		struct BindingMeta
		{
			std::string name = "";
			VkAccessFlags2 access = VK_ACCESS_NONE_KHR;
			VkImageLayout layout = VK_IMAGE_LAYOUT_MAX_ENUM;
			VkImageUsageFlags image_usage = 0;
			VkBufferUsageFlags buffer_usage = 0;
		};

	protected:

		VkDescriptorSetLayout _handle = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding> _bindings;
		std::vector<BindingMeta> _metas;

		void create(VkDescriptorSetLayoutCreateInfo const& ci);

		void destroy();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkDescriptorSetLayoutCreateFlags flags = 0;
			std::vector<VkDescriptorSetLayoutBinding> bindings = {};
			std::vector<BindingMeta> metas = {};
			VkDescriptorBindingFlags binding_flags = 0;
		};
		using CI = CreateInfo;

		DescriptorSetLayout(CreateInfo const& ci);

		virtual ~DescriptorSetLayout() override;

		constexpr decltype(auto) handle()const
		{
			return _handle;
		}

		constexpr operator VkDescriptorSetLayout()const
		{
			return handle();
		}

		constexpr VkDescriptorSetLayout descriptorSetLayout()const
		{
			return handle();
		}
		
		constexpr std::vector<VkDescriptorSetLayoutBinding> const& bindings()const
		{
			return _bindings;
		}

		constexpr const auto& metas()const
		{
			return _metas;
		}

		constexpr bool empty()const
		{
			return _bindings.empty();
		}
	};
}