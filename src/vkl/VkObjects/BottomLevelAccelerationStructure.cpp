#include <vkl/VkObjects/BottomLevelAccelerationStructure.hpp>

namespace vkl
{
	BottomLevelAccelerationStructureInstance::BottomLevelAccelerationStructureInstance(CreateInfo const& ci) :
		AccelerationStructureInstance(AccelerationStructureInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.geometry_flags = ci.geometry_flags,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
			.build_flags = ci.build_flags,
			.storage_buffer = ci.storage_buffer,
			.build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		}),
		_triangle_mesh_geometies(ci.geometries)
	{
		create();
	}

	BottomLevelAccelerationStructureInstance::~BottomLevelAccelerationStructureInstance()
	{
		// Later
		// TODO remove callbacks?
	}

	void BottomLevelAccelerationStructureInstance::create()
	{
		_vk_geometries.resize(_triangle_mesh_geometies.size());
		_max_primitive_count.resize(_triangle_mesh_geometies.size());
		for (size_t i = 0; i < _vk_geometries.size(); ++i)
		{
			const TriangleMeshGeometry& src = _triangle_mesh_geometies[i];
			_vk_geometries[i] = VkAccelerationStructureGeometryKHR{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
				.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
				.geometry = VkAccelerationStructureGeometryDataKHR{
					.triangles = VkAccelerationStructureGeometryTrianglesDataKHR{
						.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
						.pNext = nullptr,
						.vertexFormat = src.vertex_desc.format,
						.vertexData = src.vertex_buffer.buffer->deviceAddress() + src.vertex_buffer.range.begin,
						.vertexStride = src.vertex_desc.stride,
						.maxVertex = src.max_vertex,
						.indexType = src.index_type,
						.indexData = src.index_buffer.buffer ? (src.index_buffer.buffer->deviceAddress() + src.index_buffer.range.begin) : 0,
						.transformData = 0,
					},
				},
				.flags = src.flags,
			};
			_max_primitive_count[i] = src.max_primitive;
		}

		AccelerationStructureInstance::create();
	}



	void BottomLevelAccelerationStructure::createInstance()
	{
		assert(!_inst);
		BLASI::CI ci{
			.app = application(),
			.name = name(),
			.geometry_flags = _geometry_flags.valueOr(0),
			.build_flags = _build_flags,
		};

		ci.geometries.resize(_vk_geometries.size());
		for (size_t i = 0; i < _vk_geometries; ++i)
		{
			const Geometry& g = _vk_geometries[i];
			Geometry::Capacity c = g.capacity.value();
			ci.geometries[i] = BLASI::TriangleMeshGeometry{
				.vertex_buffer = g.vertex_buffer.getInstance(),
				.vertex_desc = g.vertex_description.value(),
				.index_buffer = g.index_buffer.getInstance(),
				.index_type = g.index_type.value(),
				.max_vertex = c.max_vertex,
				.max_primitive = c.max_primitives,
				.flags = ci.geometry_flags,
			};
		}

		if (_storage_buffer)
		{
			ci.storage_buffer = _storage_buffer.getInstance();
		}

		_inst = std::make_shared<BLASI>(std::move(ci));
	}

	void BottomLevelAccelerationStructure::updateResources(UpdateContext& ctx)
	{
		if (ctx.updateTick() <= _update_tick)
		{
			return;
		}
		_update_tick = ctx.updateTick();

		const VkGeometryFlagsKHR common_flags = _geometry_flags.valueOr(0);

		if (_storage_buffer)
		{
			_storage_buffer.buffer->updateResource(ctx);
		}

		if (checkHoldInstance())
		{
			bool res = false;

			if (_inst)
			{
				BottomLevelAccelerationStructureInstance& inst = *instance();

				if (inst.geometryFlags() != common_flags)
				{
					res = true;
				}
				if (!res)
				{
					if (inst.triangleMeshGeometries().size() != _vk_geometries.size())
					{
						res = true;
					}
					else
					{
						for (size_t i = 0; i < _vk_geometries.size(); ++i)
						{
							// TODO

						}
					}
				}
			}
			if (res)
			{
				destroyInstanceIFN();
			}

			if (!_inst)
			{
				createInstance();
			}
		}
	}


	BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(CreateInfo const& ci) :
		AccelerationStructure(AccelerationStructure::CI{
			.app = ci.app,
			.name = ci.name,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
			.geometry_flags = ci.geometry_flags,
			.build_flags = ci.build_flags,
			.storage_buffer = ci.storage_buffer,
			.hold_instance = ci.hold_instance,
			}),
			_vk_geometries(ci.geometries)
	{
		// TODO Keep instance if possible?
		// We could only mark the BLAS for rebuild, rather than recreate its instance?
		for (size_t i = 0; i < _vk_geometries.size(); ++i)
		{
			Callback cb{
				.callback = [this]() {
					destroyInstanceIFN();
				},
				.id = this,
			};
			Geometry& g = _vk_geometries[i];
			g.vertex_buffer.buffer->setInvalidationCallback(cb);
			if (g.index_buffer && g.index_buffer.buffer != g.vertex_buffer.buffer)
			{
				g.index_buffer.buffer->setInvalidationCallback(cb);
			}
		}
	}

	BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure()
	{
		for (size_t i = 0; i < _vk_geometries.size(); ++i)
		{
			Geometry& g = _vk_geometries[i];
			g.vertex_buffer.buffer->removeInvalidationCallback(this);
			if (g.index_buffer && g.index_buffer.buffer != g.vertex_buffer.buffer)
			{
				g.index_buffer.buffer->removeInvalidationCallback(this);
			}
		}
	}
}