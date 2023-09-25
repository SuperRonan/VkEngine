#include "DescriptorSetLayout.hpp"
#include <algorithm>
#include <cassert>

namespace vkl
{

	void DescriptorSetLayout::create(VkDescriptorSetLayoutCreateInfo const& ci)
	{
		VK_CHECK(vkCreateDescriptorSetLayout(_app->device(), &ci, nullptr, &_handle), "Failed to create a descriptor set layout.");
	}

	void DescriptorSetLayout::setVkName()
	{
		application()->nameObject(VkDebugUtilsObjectNameInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
			.objectHandle = reinterpret_cast<uint64_t>(_handle),
			.pObjectName = name().c_str(),
		});
	}

	void DescriptorSetLayout::destroy()
	{
		vkDestroyDescriptorSetLayout(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	void DescriptorSetLayout::sortBindings()
	{
		// TODO Optimize sometime
		// Make a std SoA sort
		struct Tmp {
			VkDescriptorSetLayoutBinding binding = {};
			BindingMeta meta = {};
		};
		std::vector<Tmp> tmp(_bindings.size());
		for (size_t i = 0; i < tmp.size(); ++i)
		{
			tmp[i].binding = _bindings[i];
			tmp[i].meta = _metas[i];
		}
		std::sort(tmp.begin(), tmp.end(), [](Tmp const& a, Tmp const& b){return a.binding.binding < b.binding.binding; });
		for (size_t i = 0; i < tmp.size(); ++i)
		{
			_bindings[i] = tmp[i].binding;
			_metas[i] = tmp[i].meta;
		}
	}
	
	DescriptorSetLayout::DescriptorSetLayout(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_metas(ci.metas),
		_bindings(ci.bindings)
	{

		sortBindings();

		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			const VkDescriptorType type = _bindings[i].descriptorType;
			if ( // Is image
				type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
				type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
				type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
			)
			{
				if (_metas[i].layout == VK_IMAGE_LAYOUT_UNDEFINED)
				{
					VkImageLayout induced_layout = VK_IMAGE_LAYOUT_UNDEFINED;
					if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || _metas[i].access == VK_ACCESS_2_SHADER_READ_BIT)
					{
						induced_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					}
					else 
					{
						induced_layout = VK_IMAGE_LAYOUT_GENERAL;
					}
					_metas[i].layout = induced_layout;
				}
			}
		}

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
		setVkName();
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		if (_handle)
		{
			destroy();
		}
	}
	


	MultiDescriptorSetsLayouts& MultiDescriptorSetsLayouts::operator+=(std::pair<uint32_t, std::shared_ptr<DescriptorSetLayout>> const& p)
	{
		if (p.first >= _layouts.size())
		{
			_layouts.resize(p.first + 1);
		}
		_layouts[p.first] = p.second;
		return *this;
	}

}