#pragma once

#include "VkApplication.hpp"

namespace vkl
{
	class DescriptorSetLayout : public VkObject
	{
	protected:

		VkDescriptorSetLayout _handle = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding> _bindings;
		std::vector<std::string> _names;

		void create(VkDescriptorSetLayoutCreateInfo const& ci);

		void destroy();

	public:

		constexpr DescriptorSetLayout(VkApplication * app = nullptr):
			VkObject(app)
		{}

		DescriptorSetLayout(VkApplication* app, VkDescriptorSetLayoutCreateInfo const& ci);

		DescriptorSetLayout(VkApplication* app, std::vector<VkDescriptorSetLayoutBinding> const& bindings, std::vector<std::string> const& names);

		constexpr DescriptorSetLayout(DescriptorSetLayout const&) = delete;

		constexpr DescriptorSetLayout(DescriptorSetLayout&& other) :
			VkObject(std::move(other))
		{
			_handle = other._handle;
			other._handle = VK_NULL_HANDLE;
			_bindings = std::move(other._bindings);
		}

		constexpr DescriptorSetLayout& operator=(DescriptorSetLayout const&) = delete;

		constexpr DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_handle, other._handle);
			std::swap(_bindings, other._bindings);
			return *this;
		}

		~DescriptorSetLayout();

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

		constexpr auto& names()const
		{
			return _names;
		}
	};
}