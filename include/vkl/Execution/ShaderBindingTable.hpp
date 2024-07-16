#pragma once

#include <vkl/VkObjects/RayTracingPipeline.hpp>
#include <vkl/Execution/Module.hpp>
#include <vkl/Execution/HostManagedBuffer.hpp>

#define VKL_SBT_HOST_STORAGE_UNIFIED 0
#define VKL_SBT_HOST_STORAGE_SEPARATE 1

#ifndef VKL_SBT_USE_HOST_STORAGE
#define VKL_SBT_USE_HOST_STORAGE VKL_SBT_HOST_STORAGE_UNIFIED
#endif

namespace vkl
{
	class ShaderBindingTable : public Module
	{
	public:

	protected:

		using Range = Range32u;
		
		struct Segment
		{
			// Offset in bytes
			uint32_t base = 0;
			// Max byte size of a data record (/!\ does not include the shader group handle)
			uint32_t shader_data_record_size = 0;
			// Number of shader records
			uint32_t count = 0;
			
			Range indices_invalidation = {};
			Range data_invalidation = {};

#if VKL_SBT_USE_HOST_STORAGE == VKL_SBT_HOST_STORAGE_UNIFIED
			uint32_t shader_index_base = 0;
			uint32_t shader_data_base = 0;
#else
			MyVector<uint32_t> indices;
			MyVector<uint32_t> data;
#endif
		};

		std::shared_ptr<RayTracingPipeline> _pipeline = nullptr;
		
		HostManagedBuffer _buffer;

		std::array<Segment, 4> _segments = {}; 

#if VKL_SBT_USE_HOST_STORAGE == VKL_SBT_HOST_STORAGE_UNIFIED
		MyVector<uint32_t> _shader_record_indices = {};
		MyVector<uint8_t> _shader_record_data = {};
#endif

		size_t _latest_update_tick = 0;

		VkStridedDeviceAddressRegionKHR convert(Segment const& s) const;

		void signalPipelineInvalidation();

		uint32_t prepareSize();

		void writeData();

		std::shared_ptr<AsynchTask> _update_task = nullptr;

	public:

		struct Capacity
		{
			uint32_t count = 0;
			uint32_t data_size = 0;
		};

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<RayTracingPipeline> pipeline = nullptr;
			Capacity raygen = {};
			Capacity miss = {};
			Capacity hit_group = {};
			Capacity callable = {};
			VkBufferUsageFlags extra_buffer_usage = 0;
		};
		using CI = CreateInfo;

		ShaderBindingTable(CreateInfo const& ci);

		virtual ~ShaderBindingTable() override;

		HostManagedBuffer& buffer()
		{
			return _buffer;
		}

		HostManagedBuffer const& buffer()const
		{
			return _buffer;
		}

		void updateResources(UpdateContext& ctx);

		void recordUpdateIFN(ExecutionRecorder & exec);

		VkStridedDeviceAddressRegionKHR getRaygenRegion()const;
		VkStridedDeviceAddressRegionKHR getMissRegion()const;
		VkStridedDeviceAddressRegionKHR getHitGroupRegion()const;
		VkStridedDeviceAddressRegionKHR getCallableRegion()const;

		struct Regions
		{
			VkStridedDeviceAddressRegionKHR raygen;
			VkStridedDeviceAddressRegionKHR miss;
			VkStridedDeviceAddressRegionKHR hit_group;
			VkStridedDeviceAddressRegionKHR callable;
		};

		VkStridedDeviceAddressRegionKHR getRegion(ShaderRecordType record_type) const;

		Regions getRegions() const;

		// -1 -> don't change the value
		void setRecordProperties(ShaderRecordType record_type, Capacity capacity = Capacity{ .count = uint32_t(-1), .data_size = uint32_t(-1)});

		// -1 -> don't change the shader index
		void setRecord(ShaderRecordType record_type, uint32_t index, uint32_t shader_group_index = uint32_t(-1), uint32_t data_size = 0, const void* data = nullptr);
	};
}