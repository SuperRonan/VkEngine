#include "DescriptorWriter.hpp"


namespace vkl
{
	DescriptorWriter::DescriptorWriter(CreateInfo const& ci):
		VkObject(ci.app, ci.name)
	{}

	void DescriptorWriter::reserve(size_t N)
	{
		_writes.reserve(N);
		_buffers.reserve(N);
		_images.reserve(N);
	}

	void DescriptorWriter::record()
	{
		
		bool update_uniform_buffer = false;
		bool update_storage_image = false;
		bool update_storage_buffer = false;
		bool update_sampled_image = false;

		for (size_t i = 0; i < _writes.size(); ++i)
		{
			const VkDescriptorType t = _writes[i].descriptorType;
			switch (t)
			{
				case VK_DESCRIPTOR_TYPE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				{
					uintptr_t index = (uintptr_t)_writes[i].pImageInfo;
					_writes[i].pImageInfo = _images.data() + index;
				}
				break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				{
					// Not supported yet
					uintptr_t index = (uintptr_t)_writes[i].pTexelBufferView;
				}
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
				{
					uintptr_t index = (uintptr_t)_writes[i].pBufferInfo;
					_writes[i].pBufferInfo = _buffers.data() + index;
				}
				break;
				case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
				{
					// TODO
				}
				break;
				case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
				{
					// TODO
				}
				break;
			}

			switch (t)
			{
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				{
					update_uniform_buffer = true;
				}
				break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
				{
					update_storage_buffer = true;
				}
				break;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				{
					update_storage_image = true;
				}
				break;
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				{
					update_sampled_image = true;
				}
				break;
			}
		}

		if (!_writes.empty())
		{
			bool wait = false;
			const auto& features12 = application()->availableFeatures().features_12;
			if (update_uniform_buffer && features12.descriptorBindingUniformBufferUpdateAfterBind == VK_FALSE)
			{
				wait = true;
			}
			if (update_storage_buffer && features12.descriptorBindingStorageBufferUpdateAfterBind == VK_FALSE)
			{
				wait = true;
			}
			if (update_storage_image && features12.descriptorBindingStorageImageUpdateAfterBind == VK_FALSE)
			{
				wait = false;
			}
			if (update_sampled_image && features12.descriptorBindingSampledImageUpdateAfterBind == VK_FALSE)
			{
				wait = true;
			}

			if (wait)
			{
				vkDeviceWaitIdle(device());
			}

			vkUpdateDescriptorSets(device(), _writes.size(), _writes.data(), 0, nullptr);
		}


		_writes.clear();
		_images.clear();
		_buffers.clear();
	}
}