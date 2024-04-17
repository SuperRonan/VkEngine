#include "DescriptorSetLayout.hpp"
#include <algorithm>
#include <cassert>

namespace vkl
{

	void DescriptorSetLayoutInstance::create(CreateInfo const& ci)
	{
		VkDescriptorSetLayoutCreateInfo vk_ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = _flags,
			.bindingCount = static_cast<uint32_t>(_bindings.size()),
			.pBindings = _bindings.data(),
		};
		VkDescriptorSetLayoutBindingFlagsCreateInfo bindings_flags_ci;
		std::vector<VkDescriptorBindingFlags> bindings_flags;

		if (ci.binding_flags)
		{
			VkDescriptorBindingFlags common_bindings_flags = ci.binding_flags;
			const VulkanFeatures & features = application()->availableFeatures();
			const auto& features12 = features.features_12;
			if (features12.descriptorBindingUpdateUnusedWhilePending == VK_FALSE)
			{
				common_bindings_flags = common_bindings_flags & ~VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
			}
			if (features12.descriptorBindingPartiallyBound == VK_FALSE)
			{
				common_bindings_flags = common_bindings_flags & ~VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
			}

			_binding_flags = common_bindings_flags;

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
				case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
					if (features.acceleration_structure_khr.descriptorBindingAccelerationStructureUpdateAfterBind == VK_FALSE)
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

		VK_CHECK(vkCreateDescriptorSetLayout(_app->device(), &vk_ci, nullptr, &_handle), "Failed to create a descriptor set layout.");
	}

	void DescriptorSetLayoutInstance::setVkName()
	{
		application()->nameVkObjectIFP(VkDebugUtilsObjectNameInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
			.objectHandle = reinterpret_cast<uint64_t>(_handle),
			.pObjectName = name().c_str(),
		});
	}

	void DescriptorSetLayoutInstance::destroy()
	{
		vkDestroyDescriptorSetLayout(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}


	DescriptorSetLayoutInstance::DescriptorSetLayoutInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_bindings(ci.vk_bindings),
		_metas(ci.metas),
		_flags(ci.flags),
		_binding_flags(ci.binding_flags)
	{
		create(ci);
		setVkName();
	}

	DescriptorSetLayoutInstance::DescriptorSetLayoutInstance(CreateInfo && ci) :
		AbstractInstance(ci.app, std::move(ci.name)),
		_bindings(std::move(ci.vk_bindings)),
		_metas(std::move(ci.metas)),
		_flags(ci.flags),
		_binding_flags(ci.binding_flags)
	{
		create(ci);
		setVkName();
	}

	DescriptorSetLayoutInstance::~DescriptorSetLayoutInstance()
	{
		if (_handle)
		{
			destroy();
		}
	}



	void DescriptorSetLayout::sortBindings()
	{
		std::sort(_bindings.begin(), _bindings.end(), [](Binding const& a, Binding const& b){return a.binding < b.binding;});
	}
	
	DescriptorSetLayout::DescriptorSetLayout(CreateInfo const& ci) :
		ParentType(ci.app, ci.name, ci.hold_instance),
		_is_dynamic(ci.is_dynamic),
		_bindings(ci.bindings),
		_flags(ci.flags),
		_binding_flags(ci.binding_flags)
	{
		sortBindings();

		// Deduce undefined layouts
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			const VkDescriptorType type = _bindings[i].type;
			if ( // Is image
				type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
				type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
				type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
			)
			{
				if (_bindings[i].layout == VK_IMAGE_LAYOUT_UNDEFINED)
				{
					VkImageLayout induced_layout = VK_IMAGE_LAYOUT_UNDEFINED;
					if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
					{
						induced_layout = application()->options().getLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT);
					}
					else if (type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
					{
						// What if a depth attachement?
						// What if a shared sampled / attachment layout?
						// Plus this layout has to match the one in the RenderPass
						induced_layout = application()->options().getLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
					}
					else 
					{
						induced_layout = VK_IMAGE_LAYOUT_GENERAL;
					}
					_bindings[i].layout = induced_layout;
				}
			}
		}
		
		if (!isDynamic())
		{
			createInstance();
		}
	}

	void DescriptorSetLayout::createInstance()
	{
		assert(!_inst);
		std::vector<VkDescriptorSetLayoutBinding> vk_bindings(_bindings.size());
		std::vector< DescriptorSetLayoutInstance::BindingMeta> metas(_bindings.size());
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			vk_bindings[i] = _bindings[i].getVkBinding();
			metas[i] = _bindings[i].getMeta();
		}
		_inst = std::make_shared<DescriptorSetLayoutInstance>(DescriptorSetLayoutInstance::CI{
			.app = application(),
			.name = name(),
			.flags = _flags,
			.vk_bindings = std::move(vk_bindings),
			.metas = std::move(metas),
			.binding_flags = _binding_flags,
		});
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{	
		
	}
	
	bool DescriptorSetLayout::updateResources(UpdateContext& ctx)
	{
		bool res = false;
		if (isDynamic())
		{
			if (ctx.updateTick() > _update_tick)
			{
				_update_tick = ctx.updateTick();
				if (checkHoldInstance())
				{
					if (_inst)
					{
						bool destroy_instance = false;
						for (size_t i = 0; i < _bindings.size(); ++i)
						{
							if (_bindings[i].count.value() != _inst->_bindings[i].descriptorCount)
							{
								destroy_instance = true;
								break;
							}
						}
						if (destroy_instance)
						{
							destroyInstanceIFN();
						}
					}
					if (!_inst)
					{
						createInstance();
						res = true;
					}
				}
			}
		}
		return res;
	}
}