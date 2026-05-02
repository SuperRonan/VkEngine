#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/App/VkApplication.hpp>

#include <iterator>

namespace vkl
{
	class DescriptorWriter : public VkObject
	{
	public:

		struct WriteDestination
		{
			VkDescriptorSet set = VK_NULL_HANDLE;
			uint32_t binding = 0;
			uint32_t index = 0;
			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			VkDescriptorBindingFlags flags = 0;
		};

	protected:

		MyVector<VkWriteDescriptorSet> _writes;
		MyVector<VkDescriptorImageInfo> _images;
		MyVector<VkDescriptorBufferInfo> _buffers;
		MyVector<VkWriteDescriptorSetAccelerationStructureKHR> _tlas_writes;
		MyVector<VkAccelerationStructureKHR> _tlas;

		bool _update_uniform_buffer = false;
		bool _update_storage_buffer = false;
		bool _update_sampled_image = false;
		bool _update_storage_image = false;
		bool _update_tlas = false;
		bool _require_wait = false;

		VkWriteDescriptorSet& addWrite(WriteDestination const& dst, uint32_t count);

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		void reserve(size_t N);

		void clear();

		DescriptorWriter(CreateInfo const& ci);


		VkDescriptorBufferInfo* addBuffers(WriteDestination const& dst, size_t N);
		
		void add(WriteDestination const& dst, VkDescriptorBufferInfo const& buffer_info)
		{
			*addBuffers(dst, 1) = buffer_info;
		}
		
		void add(WriteDestination const& dst, std::span<const VkDescriptorBufferInfo> buffer_infos);


		VkDescriptorImageInfo* addImages(WriteDestination const& dst, size_t N);

		void add(WriteDestination const& dst, VkDescriptorImageInfo const& image_info)
		{
			*addImages(dst, 1) = image_info;
		}

		void add(WriteDestination const& dst, std::span<VkDescriptorImageInfo const> image_infos);


		VkAccelerationStructureKHR* addTLAS(WriteDestination const& dst, uint32_t count = 1);

		void add(WriteDestination const& dst, VkAccelerationStructureKHR tlas)
		{
			*addTLAS(dst) = tlas;
		}

		void add(WriteDestination const& dst, std::span<const VkAccelerationStructureKHR> tlas);

		
		void record();
	};
}