#include "DescriptorSetLayout.hpp"

namespace vkl
{

	void DescriptorSetLayout::create(VkDescriptorSetLayoutCreateInfo const& ci)
	{
		VK_CHECK(vkCreateDescriptorSetLayout(_app->device(), &ci, nullptr, &_handle), "Failed to create a descriptor set layout.");
	}

	void DescriptorSetLayout::destroy()
	{
		vkDestroyDescriptorSetLayout(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	DescriptorSetLayout::DescriptorSetLayout(VkApplication* app, VkDescriptorSetLayoutCreateInfo const& ci) :
		VkObject(app)
	{
		create(ci);
	}
	
	DescriptorSetLayout::DescriptorSetLayout(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_metas(ci.metas),
		_bindings(ci.bindings)
	{
		VkDescriptorSetLayoutCreateInfo vk_ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = ci.flags,
			.bindingCount = static_cast<uint32_t>(_bindings.size()),
			.pBindings = _bindings.data(),
		};
		VkDescriptorSetLayoutBindingFlagsCreateInfo bindings_flags_ci;
		std::vector<VkDescriptorBindingFlags> bindings_flags;
		
		if (ci.binding_flags)
		{
			VkDescriptorBindingFlags common_bindings_flags = ci.binding_flags;
			const auto& features12 = application()->availableFeatures().features_12;
			if (features12.descriptorBindingUpdateUnusedWhilePending == VK_FALSE)
			{
				common_bindings_flags = common_bindings_flags & ~VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
			}

			bindings_flags = std::vector<VkDescriptorBindingFlags>(_bindings.size(), 0);
			for (size_t i = 0; i < _bindings.size(); ++i)
			{
				VkDescriptorBindingFlags binding_flags = common_bindings_flags;
				bool remove_update_after_bind = false;
				switch (_bindings[i].descriptorType)
				{
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					if (features12.descriptorBindingUniformBufferUpdateAfterBind == VK_FALSE)
					{
						remove_update_after_bind = true;
					}
					break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					if (features12.descriptorBindingStorageBufferUpdateAfterBind == VK_FALSE)
					{
						remove_update_after_bind = true;
					}
					break;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					if (features12.descriptorBindingStorageImageUpdateAfterBind == VK_FALSE)
					{
						remove_update_after_bind = true;
					}
					break;
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					if (features12.descriptorBindingSampledImageUpdateAfterBind == VK_FALSE)
					{
						remove_update_after_bind = true;
					}
					break;
				default:
					std::cerr << "Unknown descriptor type!" << std::endl;
				}
				if (remove_update_after_bind)
				{
					binding_flags = binding_flags & ~VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
				}

				bindings_flags[i] = binding_flags;
			}

			vk_ci.pNext = &bindings_flags_ci;
			bindings_flags_ci = VkDescriptorSetLayoutBindingFlagsCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				.pNext = nullptr,
				.bindingCount = static_cast<uint32_t>(bindings_flags.size()),
				.pBindingFlags = bindings_flags.data(),
			};
		}
		create(vk_ci);
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		if (_handle)
		{
			destroy();
		}
	}

}