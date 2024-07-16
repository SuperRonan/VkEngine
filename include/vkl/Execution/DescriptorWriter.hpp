#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/App/VkApplication.hpp>

#include <iterator>

namespace vkl
{
	class DescriptorWriter : public VkObject
	{
	protected:

		MyVector<VkWriteDescriptorSet> _writes;
		MyVector<VkDescriptorImageInfo> _images;
		MyVector<VkDescriptorBufferInfo> _buffers;
		MyVector<VkWriteDescriptorSetAccelerationStructureKHR> _tlas_writes;
		MyVector<VkAccelerationStructureKHR> _tlas;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		void reserve(size_t N);

		DescriptorWriter(CreateInfo const& ci);

		struct WriteDestination
		{
			VkDescriptorSet set = VK_NULL_HANDLE;
			uint32_t binding = 0;
			uint32_t index = 0;
			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		};

		void add(WriteDestination const& dst, VkDescriptorBufferInfo const& buffer_info)
		{
			const size_t o = _buffers.size();
			_buffers.push_back(buffer_info);
			assert(dst.type != VK_DESCRIPTOR_TYPE_MAX_ENUM);
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = dst.set,
				.dstBinding = dst.binding,
				.dstArrayElement = dst.index,
				.descriptorCount = 1,
				.descriptorType = dst.type,
				.pImageInfo = nullptr,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr,
			};
			std::uintptr_t & info_index = (std::uintptr_t&)write.pBufferInfo;
			info_index = o;
			_writes.push_back(write);
		}

		VkDescriptorBufferInfo* addBuffers(WriteDestination const& dst, size_t N)
		{
			const size_t o = _buffers.size();
			_buffers.resize(o + N);

			assert(dst.type != VK_DESCRIPTOR_TYPE_MAX_ENUM);
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = dst.set,
				.dstBinding = dst.binding,
				.dstArrayElement = dst.index,
				.descriptorCount = static_cast<uint32_t>(N),
				.descriptorType = dst.type,
				.pImageInfo = nullptr,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr,
			};
			std::uintptr_t& info_index = (std::uintptr_t&)write.pBufferInfo;
			info_index = o;
			_writes.push_back(write);

			return _buffers.data() + o;
		}
		
		template <std::forward_iterator It, class ItEnd = It>
		void addBuffers(WriteDestination const& dst, It buffer_info_begin, ItEnd const& buffer_info_end)
		{
			auto& it = buffer_info_begin;
			const size_t o = _buffers.size();
			size_t N = 0;
			if constexpr (std::random_access_iterator<It>)
			{
				N = std::distance(buffer_info_begin, buffer_info_end);
				_buffers.resize(o + N);
				std::copy_n(buffer_info_begin, N, _buffers.data() + o);
			}
			else
			{
				while (it != buffer_info_end)
				{
					_buffers.push_back(*it);
					it = std::next(it);
				}
				N = _buffers.size() - o;
			}
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = dst.set,
				.dstBinding = dst.binding,
				.dstArrayElement = dst.index,
				.descriptorCount = static_cast<uint32_t>(N),
				.descriptorType = dst.type,
				.pImageInfo = nullptr,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr,
			};
			std::uintptr_t& info_index = (std::uintptr_t&)write.pBufferInfo;
			info_index = o;
			_writes.push_back(write);
		}
		
		void add(WriteDestination const& dst, std::vector<VkDescriptorBufferInfo> const& buffer_infos)
		{
			addBuffers(dst, buffer_infos.cbegin(), buffer_infos.cend());
		}

		void add(WriteDestination const& dst, VkDescriptorImageInfo const& image_info)
		{
			const size_t o = _images.size();
			_images.push_back(image_info);
			assert(dst.type != VK_DESCRIPTOR_TYPE_MAX_ENUM);
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = dst.set,
				.dstBinding = dst.binding,
				.dstArrayElement = dst.index,
				.descriptorCount = 1,
				.descriptorType = dst.type,
				.pImageInfo = nullptr,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr,
			};
			std::uintptr_t& info_index = (std::uintptr_t&)write.pImageInfo;
			info_index = o;
			_writes.push_back(write);
		}

		VkDescriptorImageInfo* addImages(WriteDestination const& dst, size_t N)
		{
			const size_t o = _images.size();
			_images.resize(o + N);

			assert(dst.type != VK_DESCRIPTOR_TYPE_MAX_ENUM);
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = dst.set,
				.dstBinding = dst.binding,
				.dstArrayElement = dst.index,
				.descriptorCount = static_cast<uint32_t>(N),
				.descriptorType = dst.type,
				.pImageInfo = nullptr,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr,
			};
			std::uintptr_t& info_index = (std::uintptr_t&)write.pImageInfo;
			info_index = o;
			_writes.push_back(write);

			return _images.data() + o;
		}
		
		template <std::forward_iterator It, class ItEnd = It>
		void addImages(WriteDestination const& dst, It image_info_begin, ItEnd const& image_info_end)
		{
			auto& it = image_info_begin;
			const size_t o = _images.size();
			size_t N = 0;
			if constexpr (std::random_access_iterator<It>)
			{
				N = std::distance(image_info_begin, image_info_end);
				_images.resize(o + N);
				std::copy_n(image_info_begin, N, _images.data() + o);
			}
			else
			{
				while (it != image_info_end)
				{
					_images.push_back(*it);
					it = std::next(it);
				}
				N = _images.size() - o;
			}
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = dst.set,
				.dstBinding = dst.binding,
				.dstArrayElement = dst.index,
				.descriptorCount = static_cast<uint32_t>(N),
				.descriptorType = dst.type,
				.pImageInfo = nullptr,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr,
			};
			std::uintptr_t& info_index = (std::uintptr_t&)write.pImageInfo;
			info_index = o;
			_writes.push_back(write);
		}
		
		void add(WriteDestination const& dst, std::vector<VkDescriptorImageInfo> const& image_infos)
		{
			addImages(dst, image_infos.cbegin(), image_infos.cend());
		}

		VkAccelerationStructureKHR* addTLAS(WriteDestination const& dst, uint32_t count = 1)
		{
			assert(count > 0);
			assert(dst.type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
			_writes.push_back(VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = dst.set,
				.dstBinding = dst.binding,
				.dstArrayElement = dst.index,
				.descriptorCount = count,
				.descriptorType = dst.type,
				.pImageInfo = nullptr,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr,
			});
			std::uintptr_t & pn_index = (std::uintptr_t&)_writes.back().pNext;
			pn_index = _tlas_writes.size();
			_tlas_writes.push_back(VkWriteDescriptorSetAccelerationStructureKHR{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
				.pNext = nullptr,
				.accelerationStructureCount = count,
				.pAccelerationStructures = 0,
			});
			const size_t o = _tlas.size();
			std::uintptr_t & index = (std::uintptr_t&)_tlas_writes.back().pAccelerationStructures;
			index = o;
			_tlas.resize(o + count);
			return _tlas.data() + o;
		}

		void record();
	};
}