#include <vkl/Execution/DescriptorWriter.hpp>

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

	VkWriteDescriptorSet& DescriptorWriter::addWrite(WriteDestination const& dst, uint32_t count)
	{
		assert(dst.type != VK_DESCRIPTOR_TYPE_MAX_ENUM);
		_writes.push_back(VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = dst.set,
			.dstBinding = dst.binding,
			.dstArrayElement = dst.index,
			.descriptorCount = static_cast<uint32_t>(count),
			.descriptorType = dst.type,
			.pImageInfo = nullptr,
			.pBufferInfo = nullptr,
			.pTexelBufferView = nullptr,
		});
		VkWriteDescriptorSet& write = _writes.back();
		if (!(dst.flags & VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT))
		{
			_require_wait |= true;
		}
		return write;
	}

	VkDescriptorBufferInfo* DescriptorWriter::addBuffers(WriteDestination const& dst, size_t N)
	{
		if (N == 0)
		{
			return nullptr;
		}
		VkWriteDescriptorSet& write = addWrite(dst, N);
		const size_t o = _buffers.size();
		reinterpret_cast<std::uintptr_t&>(write.pBufferInfo) = o;
		_buffers.resize(o + N);
		
		switch (dst.type)
		{
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			_update_uniform_buffer |= true;
			break;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			_update_storage_buffer |= true;
			break;
		default:
			VKL_SHOULD_NOT_BE_HERE;
			break;
		}

		return _buffers.data() + o;
	}

	void DescriptorWriter::add(WriteDestination const& dst, std::span<const VkDescriptorBufferInfo> buffer_infos)
	{
		VkDescriptorBufferInfo* target = addBuffers(dst, buffer_infos.size());
		std::copy_n(buffer_infos.data(), buffer_infos.size(), target);
	}

	VkDescriptorImageInfo* DescriptorWriter::addImages(WriteDestination const& dst, size_t N)
	{
		if (N == 0)
		{
			return nullptr;
		}
		VkWriteDescriptorSet& write = addWrite(dst, N);
		const size_t o = _images.size();
		reinterpret_cast<std::uintptr_t&>(write.pImageInfo) = o;
		_images.resize(o + N);

		switch (dst.type)
		{
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				_update_sampled_image |= true;
			break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				_update_sampled_image |= true;
			break;
			default:
				VKL_SHOULD_NOT_BE_HERE;
			break;
		}

		return _images.data() + o;
	}

	void DescriptorWriter::add(WriteDestination const& dst, std::span<VkDescriptorImageInfo const> image_infos)
	{
		VkDescriptorImageInfo* target = addImages(dst, image_infos.size());
		std::copy_n(image_infos.data(), image_infos.size(), target);
	}

	VkAccelerationStructureKHR* DescriptorWriter::addTLAS(WriteDestination const& dst, uint32_t count)
	{
		if (count == 0)
		{
			return nullptr;
		}
		VkWriteDescriptorSet& write = addWrite(dst, count);
		const size_t o = _tlas_writes.size();
		const size_t q = _tlas.size();
		reinterpret_cast<std::uintptr_t&>(write.pNext) = o;
		_tlas_writes.push_back(VkWriteDescriptorSetAccelerationStructureKHR {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
			.pNext = nullptr,
			.accelerationStructureCount = static_cast<uint32_t>(count),
			.pAccelerationStructures = reinterpret_cast<const VkAccelerationStructureKHR*>(q),
		});
		_tlas.resize(q + count);

		_update_tlas |= true;

		return _tlas.data() + o;
	}

	void DescriptorWriter::add(WriteDestination const& dst, std::span<const VkAccelerationStructureKHR> tlas)
	{
		VkAccelerationStructureKHR* target = addTLAS(dst, tlas.size());
		std::copy_n(tlas.data(), tlas.size(), target);
	}

	void DescriptorWriter::record()
	{

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
				break;
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
					const std::uintptr_t as_write_index = (std::uintptr_t)_writes[i].pNext;
					_writes[i].pNext = _tlas_writes.data() + as_write_index;
					const std::uintptr_t tlas_index = (std::uintptr_t)_tlas_writes[as_write_index].pAccelerationStructures;
					_tlas_writes[as_write_index].pAccelerationStructures = _tlas.data() + tlas_index;
				}
				break;
			}
		}

		if (!_writes.empty())
		{
			bool wait = _require_wait;
			if (!wait)
			{
				const auto & features = application()->availableFeatures();
				const auto& features12 = features.features_12;
				if (_update_uniform_buffer && features12.descriptorBindingUniformBufferUpdateAfterBind == VK_FALSE)
				{
					wait = true;
				}
				if (_update_storage_buffer && features12.descriptorBindingStorageBufferUpdateAfterBind == VK_FALSE)
				{
					wait = true;
				}
				if (_update_storage_image && features12.descriptorBindingStorageImageUpdateAfterBind == VK_FALSE)
				{
					wait = false;
				}
				if (_update_sampled_image && features12.descriptorBindingSampledImageUpdateAfterBind == VK_FALSE)
				{
					wait = true;
				}
				if (_update_tlas && features.acceleration_structure_khr.descriptorBindingAccelerationStructureUpdateAfterBind == VK_FALSE)
				{
					wait = true;
				}
			}

			if (wait)
			{
				application()->deviceWaitIdle();
			}

			vkUpdateDescriptorSets(device(), _writes.size(), _writes.data(), 0, nullptr);
		}

		clear();	
	}

	void DescriptorWriter::clear()
	{
		_writes.clear();
		_images.clear();
		_buffers.clear();
		_tlas_writes.clear();
		_tlas.clear();

		_update_uniform_buffer = false;
		_update_storage_buffer = false;
		_update_sampled_image = false;
		_update_storage_image = false;
		_update_tlas = false;
		_require_wait = false;
	}
}