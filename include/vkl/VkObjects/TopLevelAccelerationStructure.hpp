#pragma once

#include "BottomLevelAccelerationStructure.hpp"

namespace vkl
{
	class TopLevelAccelerationStructureInstance : public AccelerationStructureInstance
	{
	public:

		struct BLASInstance
		{
			std::shared_ptr<BottomLevelAccelerationStructure> blas = nullptr;
			std::shared_ptr<BottomLevelAccelerationStructureInstance> blasi = nullptr;
			VkTransformMatrixKHR xform;
			uint32_t                      instanceCustomIndex : 24;
			uint32_t                      mask : 8;
			uint32_t                      instanceShaderBindingTableRecordOffset : 24;
			VkGeometryInstanceFlagsKHR    flags : 8;
			uint32_t unique_id;
		};

		struct Geometry
		{
			BufferAndRangeInstance instances_buffer;
			MyVector<BLASInstance> blases;

			VkGeometryFlagsKHR flags = 0;
			uint32_t primitive_count = 0;
		};

	protected:

		MyVector<Geometry> _geometries;

		virtual void create() override;

	public:

		struct GeometryCreateInfo
		{
			VkGeometryFlagsKHR flags = 0;
			uint32_t capacity = 0;
			BufferAndRangeInstance instances_buffer;
		};

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkGeometryFlagsKHR geometry_flags = 0;
			VkBuildAccelerationStructureFlagsKHR build_flags;
			MyVector<GeometryCreateInfo> geometries = {};
			BufferAndRangeInstance storage_buffer = {};
			VkBuildAccelerationStructureModeKHR build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;
		};
		using CI = CreateInfo;

		void write();

		TopLevelAccelerationStructureInstance(CreateInfo const& ci);

		virtual ~TopLevelAccelerationStructureInstance() override;

		void link();

		MyVector<Geometry>& geometries()
		{
			return _geometries;
		}

		MyVector<Geometry> const& geometries()const
		{
			return _geometries;
		}

		void setPrimitiveCountIFN(uint32_t geometry_id, uint32_t n);

	};
	using TLASI = TopLevelAccelerationStructureInstance;

	class TopLevelAccelerationStructure : public AccelerationStructure
	{
	public:

		struct BLASInstance
		{
			std::shared_ptr<BottomLevelAccelerationStructure> blas = nullptr;
			VkTransformMatrixKHR xform;
			uint32_t                      instanceCustomIndex : 24;
			uint32_t                      mask : 8;
			uint32_t                      instanceShaderBindingTableRecordOffset : 24;
			VkGeometryInstanceFlagsKHR    flags : 8;
			bool _mark_for_update = false;
			uint32_t _compact_id = uint32_t(-1);
			std::shared_ptr<BottomLevelAccelerationStructureInstance> _blas_instance = nullptr;

			void setXForm(Matrix4x3f const& m)
			{
				xform = ConvertXFormToVk(m);
				_mark_for_update = true;
			}
		};

		struct Geometry
		{
			Dyn<VkGeometryFlagsKHR> flags = {};
			// Sparse vector
			// Consider it more as a map rather than an array
			MyVector<BLASInstance> blases;

			// Compact array
			std::shared_ptr<HostManagedBuffer> instances_buffer = nullptr;
		};

	protected:


		MyVector<Geometry> _geometries;

	public:

		struct GeometryCreateInfo
		{
			Dyn<VkGeometryFlagsKHR> flags = {};
			uint32_t capacity = 16;
		};

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Dyn<VkGeometryFlagsKHR> geometry_flags = {};
			MyVector<GeometryCreateInfo> geometries = {};
			VkBuildAccelerationStructureFlagsKHR build_flags = 0;
			BufferAndRange storage_buffer;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		TopLevelAccelerationStructure(CreateInfo const& ci);

		virtual ~TopLevelAccelerationStructure() override;

		std::shared_ptr<TopLevelAccelerationStructureInstance> instance() const
		{
			return std::reinterpret_pointer_cast<TopLevelAccelerationStructureInstance>(_inst);
		}

		virtual void updateResources(UpdateContext& ctx) override;

		void createInstance();

		void recordTransferIFN(ExecutionRecorder& exec);

		MyVector<Geometry>& geometries()
		{
			return _geometries;
		}

		MyVector<Geometry> const& geometries()const
		{
			return _geometries;
		}

		void registerBLAS(uint32_t geometry_index, uint32_t index, BLASInstance const& blas_instance);
	};
	using TLAS = TopLevelAccelerationStructure;
}