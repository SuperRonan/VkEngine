#pragma once

#include <vkl/App/VkApplication.hpp>

#include <vkl/VkObjects/Buffer.hpp>
#include <vkl/Execution/HostManagedBuffer.hpp>

#include <vkl/Maths/Transforms.hpp>

namespace vkl
{
	struct VertexDescriptionAS
	{
		VkFormat format;
		VkDeviceSize stride;
	};

	class AccelerationStructureInstance : public AbstractInstance
	{
	public:


	protected:
		
		MyVector<VkAccelerationStructureGeometryKHR> _vk_geometries;
		MyVector<uint32_t> _max_primitive_count;
		VkGeometryFlagsKHR _geometry_flags = 0;
		VkBuildAccelerationStructureFlagsKHR  _build_flags = 0;
		VkAccelerationStructureBuildGeometryInfoKHR _build_geometry_info = {};
		VkAccelerationStructureBuildSizesInfoKHR _build_sizes = {};

		BufferAndRangeInstance _storage_buffer = {};

		VkAccelerationStructureCreateInfoKHR _ci = {};

		VkAccelerationStructureTypeKHR _type;
		VkAccelerationStructureKHR _handle = VK_NULL_HANDLE;
		VkDeviceAddress _address = 0;

		VkBuildAccelerationStructureModeKHR _build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;

		virtual void create();

		virtual void destroy();

		void setVkName();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkGeometryFlagsKHR geometry_flags = 0;
			VkAccelerationStructureTypeKHR type = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
			VkBuildAccelerationStructureFlagsKHR build_flags = 0;
			BufferAndRangeInstance storage_buffer;
			VkBuildAccelerationStructureModeKHR build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;
		};
		using CI = CreateInfo;

		AccelerationStructureInstance(CreateInfo const& ci);

		virtual ~AccelerationStructureInstance() override;

		constexpr VkGeometryFlagsKHR geometryFlags() const
		{
			return _geometry_flags;
		}

		constexpr VkAccelerationStructureTypeKHR type()const
		{
			return _type;
		}

		constexpr operator VkAccelerationStructureKHR () const
		{
			return _handle;
		}

		constexpr VkAccelerationStructureKHR handle()const
		{
			return _handle;
		}

		constexpr VkDeviceAddress address()const
		{
			return _address;
		}

		BufferAndRangeInstance const& storageBuffer()const
		{
			return _storage_buffer;
		}

		VkAccelerationStructureBuildSizesInfoKHR const& buildSizes()const
		{
			return _build_sizes;
		}

		VkAccelerationStructureBuildGeometryInfoKHR const& buildInfo()const
		{
			return _build_geometry_info;
		}

		MyVector<uint32_t> const& maxPrimitiveCount()const
		{
			return _max_primitive_count;
		}

		constexpr VkBuildAccelerationStructureModeKHR buildMode()const
		{
			return _build_mode;
		}

		void setBuildMode(VkBuildAccelerationStructureModeKHR mode)
		{
			_build_mode = mode;
		}

		void requireBuildMode(VkBuildAccelerationStructureModeKHR mode)
		{
			_build_mode = std::min(_build_mode, mode);
		}

		void requireUpdate()
		{
			requireBuildMode(VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR);
		}

		void requireRebuild()
		{
			setBuildMode(VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR);
		}

		MyVector<VkAccelerationStructureGeometryKHR> const& vkGeometries() const
		{
			return _vk_geometries;
		}

		MyVector<VkAccelerationStructureGeometryKHR> & vkGeometries() 
		{
			return _vk_geometries;
		}
	};

	class AccelerationStructure : public InstanceHolder<AccelerationStructureInstance>
	{
	public:

	protected:

		size_t _update_tick = 0;

		VkAccelerationStructureTypeKHR _type;
		BufferAndRange _storage_buffer;

		Dyn<VkGeometryFlagsKHR> _geometry_flags;
		VkBuildAccelerationStructureFlagsKHR _build_flags;
		

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkAccelerationStructureTypeKHR type = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
			Dyn<VkGeometryFlagsKHR> geometry_flags = 0;
			VkBuildAccelerationStructureFlagsKHR build_flags = 0;
			BufferAndRange storage_buffer;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		AccelerationStructure(CreateInfo const& ci);

		virtual ~AccelerationStructure() override;

		virtual void updateResources(UpdateContext& ctx) = 0;
	};



}