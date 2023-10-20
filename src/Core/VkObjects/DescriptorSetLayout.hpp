#pragma once

#include <Core/App/VkApplication.hpp>
#include <concepts>

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

		struct Binding
		{
			VkDescriptorSetLayoutBinding vk_binding;
			BindingMeta meta;
		};

	protected:

		VkDescriptorSetLayout _handle = VK_NULL_HANDLE;
		// Sorted
		std::vector<VkDescriptorSetLayoutBinding> _bindings;
		std::vector<BindingMeta> _metas;

		VkDescriptorSetLayoutCreateFlags _flags = 0;
		VkDescriptorBindingFlags _binding_flags = 0;

		void create(VkDescriptorSetLayoutCreateInfo const& ci);

		void setVkName();

		void destroy();

		void sortBindings();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkDescriptorSetLayoutCreateFlags flags = 0;
			std::vector<Binding> bindings = {};
			std::vector<VkDescriptorSetLayoutBinding> vk_bindings = {};
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

		constexpr VkDescriptorSetLayoutCreateFlags flags()const
		{
			return _flags;
		}

		constexpr VkDescriptorBindingFlags bindingFlags()const
		{
			return _binding_flags;
		}
	};

	class MultiDescriptorSetsLayouts
	{
	protected:

		std::vector<std::shared_ptr<DescriptorSetLayout>> _layouts;

	public:

		void resize(size_t s)
		{
			_layouts.resize(s);
		}

		void clear()
		{
			_layouts.clear();
		}

		size_t size()const
		{
			return _layouts.size();
		}

		std::shared_ptr<DescriptorSetLayout> getSafe(size_t i) const
		{
			if (i >= _layouts.size())	return nullptr;
			return _layouts[i];
		}

		std::shared_ptr<DescriptorSetLayout> operator[](size_t i) const
		{
			return getSafe(i);
		}

		std::shared_ptr<DescriptorSetLayout>& getRef(size_t i)
		{
			assert(i < _layouts.size());
			return _layouts[i];
		}

		void setLayout(uint32_t i, std::shared_ptr<DescriptorSetLayout> const& l)
		{
			(*this) += {i, l};
		}

		MultiDescriptorSetsLayouts& operator+=(std::pair<uint32_t, std::shared_ptr<DescriptorSetLayout>> const& p);

		MultiDescriptorSetsLayouts operator+(std::pair<uint32_t, std::shared_ptr<DescriptorSetLayout>> const& p) const
		{
			MultiDescriptorSetsLayouts res = *this;
			res += p;
			return res;
		}

		std::vector<std::shared_ptr<DescriptorSetLayout>> asVector()const
		{
			return _layouts;
		}
	};
}