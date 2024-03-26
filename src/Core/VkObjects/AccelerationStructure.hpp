#pragma once

#include <Core/App/VkApplication.hpp>

#include <Core/VkObjects/Buffer.hpp>
#include <Core/Execution/HostManagedBuffer.hpp>

#include <Core/Maths/Transforms.hpp>

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
		
		MyVector<VkAccelerationStructureGeometryKHR> _geometries;
		MyVector<uint32_t> _max_primitive_count;
		VkGeometryFlagsKHR _geometry_flags = 0;
		VkBuildAccelerationStructureFlagsKHR  _build_flags = 0;
		VkAccelerationStructureBuildGeometryInfoKHR _build_geometry_info;
		VkAccelerationStructureBuildSizesInfoKHR _build_sizes;

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
			VkAccelerationStructureTypeKHR type = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
			VkGeometryFlagsKHR geometry_flags = 0;
			VkBuildAccelerationStructureFlagsKHR build_flags = 0;
			BufferAndRangeInstance storage_buffer;
			VkBuildAccelerationStructureModeKHR build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;
		};
		using CI = CreateInfo;

		AccelerationStructureInstance(CreateInfo const& ci);

		virtual ~AccelerationStructureInstance() override;

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
		};
		using CI = CreateInfo;

		AccelerationStructure(CreateInfo const& ci);

		virtual ~AccelerationStructure() override;

		virtual void updateResources(UpdateContext& ctx) = 0;

		virtual void destroyInstance();
	};





	class BottomLevelAccelerationStructureInstance : public AccelerationStructureInstance
	{
	public:

		struct TriangleMeshGeometry
		{
			BufferAndRangeInstance vertex_buffer;
			VertexDescriptionAS vertex_desc;
			BufferAndRangeInstance index_buffer;
			VkIndexType index_type;
			uint32_t max_vertex;
			uint32_t max_primitive;
		};

	protected:

		// same size as _geometries of the parent type
		MyVector<TriangleMeshGeometry> _triangle_mesh_geometies;
		
		virtual void create() override;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkGeometryFlagsKHR geometry_flags = 0;
			VkBuildAccelerationStructureFlagsKHR build_flags = 0;
			MyVector<TriangleMeshGeometry> geometries = {};
			BufferAndRangeInstance storage_buffer; // Optional, if not provided, this will allocate its own
		};
		using CI = CreateInfo;

		BottomLevelAccelerationStructureInstance(CreateInfo const& ci);

		virtual ~BottomLevelAccelerationStructureInstance() override;

		const MyVector<TriangleMeshGeometry>& triangleMeshGeometries() const
		{
			return _triangle_mesh_geometies;
		}
	};
	using BLASI = BottomLevelAccelerationStructureInstance;


	class BottomLevelAccelerationStructure : public AccelerationStructure
	{
	public:

		


		struct Geometry
		{
			struct Capacity
			{
				uint32_t max_vertex;
				uint32_t max_primitives;
			};

			BufferAndRange vertex_buffer;
			Dyn<VertexDescriptionAS> vertex_description;
			BufferAndRange index_buffer;
			Dyn<VkIndexType> index_type;
			Dyn<Capacity> capacity;
		};

	protected:
		
		MyVector<Geometry> _geometries;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Dyn<VkGeometryFlagsKHR> geometry_flags = 0;
			VkBuildAccelerationStructureFlagsKHR build_flags = 0;
			MyVector<Geometry> geometries = {};
			BufferAndRange storage_buffer;
		};
		using CI = CreateInfo;
		
		BottomLevelAccelerationStructure(CreateInfo const& ci);

		virtual ~BottomLevelAccelerationStructure() override;

		std::shared_ptr<BottomLevelAccelerationStructureInstance> instance() const
		{
			return std::reinterpret_pointer_cast<BottomLevelAccelerationStructureInstance>(_inst);
		}

		virtual void updateResources(UpdateContext& ctx) override;

		void createInstance();
	};
	using BLAS = BottomLevelAccelerationStructure;



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

	protected:
		
		BufferAndRangeInstance _instances_buffer;

		MyVector<BLASInstance> _blases;

		uint32_t _primitive_count = 0;

		virtual void create() override;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkGeometryFlagsKHR geometry_flags = 0;
			VkBuildAccelerationStructureFlagsKHR build_flags;
			MyVector<uint32_t> capacities = {};
			BufferAndRangeInstance instances_buffer = {};
			BufferAndRangeInstance storage_buffer = {};
			VkBuildAccelerationStructureModeKHR build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;
		};
		using CI = CreateInfo;

		void write();

		TopLevelAccelerationStructureInstance(CreateInfo const& ci);

		virtual ~TopLevelAccelerationStructureInstance() override;

		void link();

		BufferAndRangeInstance const& instancesBuffer()const
		{
			return _instances_buffer;
		}

		uint32_t primitiveCount()const
		{
			return _primitive_count;
		}

		void setPrimitiveCountIFN(uint32_t n)
		{
			if (_primitive_count != n)
			{
				requireRebuild();
				_primitive_count = n;
			}
		}
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

			void setXForm(Matrix4x3f const& m)
			{
				xform = convertXFormToVk(m);
				_mark_for_update = true;
			}
		};

	protected:

		// Sparse vector
		// Consider it as a map rather than an array
		MyVector<BLASInstance> _blases;

		// Compact array
		HostManagedBuffer _instances_buffer;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Dyn<VkGeometryFlagsKHR> geometry_flags = 0;
			VkBuildAccelerationStructureFlagsKHR build_flags = 0;
			BufferAndRange storage_buffer;
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

		MyVector<BLASInstance> const& blases()const
		{
			return _blases;
		}

		MyVector<BLASInstance> & blases()
		{
			return _blases;
		}

		void registerBLAS(uint32_t index, BLASInstance const& blas_instance);
	};
	using TLAS = TopLevelAccelerationStructure;

}