#pragma once

#include "VkApplication.hpp"

namespace vkl
{
	class DescriptorSetLayout : public VkObject
	{
	public:

		struct BindingMeta
		{
			std::string name = "";
			VkAccessFlags access = VK_ACCESS_NONE_KHR;
			VkImageLayout layout = VK_IMAGE_LAYOUT_MAX_ENUM;
		};

	protected:

		VkDescriptorSetLayout _handle = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding> _bindings;
		std::vector<BindingMeta> _metas;

		void create(VkDescriptorSetLayoutCreateInfo const& ci);

		void destroy();

	public:

		constexpr DescriptorSetLayout(VkApplication * app = nullptr):
			VkObject(app)
		{}

		DescriptorSetLayout(VkApplication* app, VkDescriptorSetLayoutCreateInfo const& ci);

		DescriptorSetLayout(VkApplication* app, std::vector<VkDescriptorSetLayoutBinding> const& bindings, std::vector<BindingMeta> const& metas);

		constexpr DescriptorSetLayout(DescriptorSetLayout const&) = delete;

		constexpr DescriptorSetLayout(DescriptorSetLayout&& other) noexcept:
			VkObject(std::move(other)), 
			_handle(other._handle),
			_bindings(std::move(other._bindings)),
			_metas(std::move(other._metas))
		{
			other._handle = VK_NULL_HANDLE;
		}

		constexpr DescriptorSetLayout& operator=(DescriptorSetLayout const&) = delete;

		constexpr DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_handle, other._handle);
			std::swap(_bindings, other._bindings);
			std::swap(_metas, other._metas);
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

		constexpr auto& metas()const
		{
			return _metas;
		}
	};
}