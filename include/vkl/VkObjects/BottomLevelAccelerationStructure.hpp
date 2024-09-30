#pragma once

#include "AccelerationStructure.hpp"

namespace vkl
{
	struct VertexDescriptionAS
	{
		VkFormat format;
		VkDeviceSize stride;
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
			VkGeometryFlagsKHR flags;
		};

	protected:

		// same size as _vk_geometries of the parent type
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

		MyVector<TriangleMeshGeometry>& triangleMeshGeometries()
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

		MyVector<Geometry> _vk_geometries;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Dyn<VkGeometryFlagsKHR> geometry_flags = 0;
			VkBuildAccelerationStructureFlagsKHR build_flags = 0;
			MyVector<Geometry> geometries = {};
			BufferAndRange storage_buffer;
			Dyn<bool> hold_instance = true;
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
}