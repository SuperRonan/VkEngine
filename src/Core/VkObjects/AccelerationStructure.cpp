#include <Core/VkObjects/AccelerationStructure.hpp>

namespace vkl
{

	void AccelerationStructureInstance::destroy()
	{
		assert(_handle);
		callDestructionCallbacks();
		application()->extFunctions()._vkDestroyAccelerationStructureKHR(device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
		_address = 0;
	}

	void AccelerationStructureInstance::setVkName()
	{
		application()->nameVkObjectIFP(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, reinterpret_cast<uint64_t>(_handle), name());
	}

	AccelerationStructureInstance::AccelerationStructureInstance(CreateInfo const& ci):
		AbstractInstance(ci.app, ci.name),
		_type(ci.type),
		_geometry_flags(ci.geometry_flags),
		_build_flags(ci.build_flags),
		_storage_buffer(ci.storage_buffer),
		_build_mode(ci.build_mode)
	{}

	AccelerationStructureInstance::~AccelerationStructureInstance()
	{
		if (_handle)
		{
			destroy();
		}
	}

	void AccelerationStructureInstance::create()
	{
		// Assume geometries are filled 

		_build_geometry_info = VkAccelerationStructureBuildGeometryInfoKHR{
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
			.pNext = nullptr,
			.type = _type,
			.flags = _build_flags,
			.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
			.srcAccelerationStructure = VK_NULL_HANDLE,
			.dstAccelerationStructure = VK_NULL_HANDLE,
			.geometryCount = _geometries.size32(),
			.pGeometries = _geometries.data(),
			.ppGeometries = nullptr,
			.scratchData = 0,
		};

		_build_sizes = VkAccelerationStructureBuildSizesInfoKHR{
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
			.pNext = nullptr,
		};
		assert(_geometries.size() == _max_primitive_count.size());
		application()->extFunctions()._vkGetAccelerationStructureBuildSizesKHR(device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &_build_geometry_info, _max_primitive_count.data(), &_build_sizes);

		if (!_storage_buffer.buffer)
		{
			_storage_buffer.buffer = std::make_shared<BufferInstance>(BufferInstance::CI{
				.app = application(),
				.name = name() + ".StorageBuffer",
				.ci = VkBufferCreateInfo{
					.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.size = _build_sizes.accelerationStructureSize,
					.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
					.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
					.queueFamilyIndexCount = 0,
					.pQueueFamilyIndices = nullptr,
				},
				.aci = VmaAllocationCreateInfo{
					.usage = VMA_MEMORY_USAGE_GPU_ONLY,
				},
				.allocator = application()->allocator(),
			});
			_storage_buffer.range = Buffer::Range{.begin = 0, .len = _build_sizes.accelerationStructureSize};
		}
		else
		{
			assert(_storage_buffer.range.len >= _build_sizes.accelerationStructureSize);
		}

		_ci = VkAccelerationStructureCreateInfoKHR{
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.createFlags = 0,
			.buffer = _storage_buffer.buffer->handle(),
			.offset = _storage_buffer.range.begin,
			.size = _storage_buffer.range.len,
			.type = _type,
			.deviceAddress = 0,
		};

		application()->extFunctions()._vkCreateAccelerationStructureKHR(device(), &_ci, nullptr, &_handle);

		VkAccelerationStructureDeviceAddressInfoKHR address_info{
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
			.pNext = nullptr,
			.accelerationStructure = _handle,
		};
		_address = application()->extFunctions()._vkGetAccelerationStructureDeviceAddressKHR(device(), &address_info);

		setVkName();
	}

	

	AccelerationStructure::AccelerationStructure(CreateInfo const& ci):
		InstanceHolder<AccelerationStructureInstance>(ci.app, ci.name),
		_type(ci.type),
		_geometry_flags(ci.geometry_flags),
		_build_flags(ci.build_flags),
		_storage_buffer(ci.storage_buffer)
	{

	}

	AccelerationStructure::~AccelerationStructure()
	{
		destroyInstance();
	}

	void AccelerationStructure::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}






	BottomLevelAccelerationStructureInstance::BottomLevelAccelerationStructureInstance(CreateInfo const& ci) :
		AccelerationStructureInstance(AccelerationStructureInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
			.geometry_flags = ci.geometry_flags,
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
		_geometries.resize(_triangle_mesh_geometies.size());
		_max_primitive_count.resize(_triangle_mesh_geometies.size());
		for (size_t i = 0; i < _geometries.size(); ++i)
		{
			const TriangleMeshGeometry & src = _triangle_mesh_geometies[i];
			_geometries[i] = VkAccelerationStructureGeometryKHR{
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
				.flags = _geometry_flags,
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
			.geometry_flags = _geometry_flags.value(),
			.build_flags = _build_flags,
		};

		ci.geometries.resize(_geometries.size());
		for (size_t i = 0; i < _geometries; ++i)
		{
			const Geometry & g = _geometries[i];
			Geometry::Capacity c = g.capacity.value();
			ci.geometries[i] = BLASI::TriangleMeshGeometry{
				.vertex_buffer = g.vertex_buffer.getInstance(),
				.vertex_desc = g.vertex_description.value(),
				.index_buffer = g.index_buffer.getInstance(),
				.index_type = g.index_type.value(),
				.max_vertex = c.max_vertex,
				.max_primitive = c.max_primitives,
			};
		}

		if (_storage_buffer)
		{
			ci.storage_buffer = _storage_buffer.getInstance();
		}

		_inst = std::make_shared<BLASI>(ci);
	}

	void BottomLevelAccelerationStructure::updateResources(UpdateContext & ctx)
	{
		if (ctx.updateTick() <= _update_tick)
		{
			return;
		}
		_update_tick = ctx.updateTick();

		if (_storage_buffer)
		{
			_storage_buffer.buffer->updateResource(ctx);
		}

		bool res = false;

		if (_inst)
		{
			BottomLevelAccelerationStructureInstance & inst = *instance();
			if (inst.triangleMeshGeometries().size() != _geometries.size())
			{
				res = true;
			}
			else
			{
				for (size_t i = 0; i < _geometries.size(); ++i)
				{
					// TODO

				}
			}
		}
		if (res)
		{
			destroyInstance();
		}

		if (!_inst)
		{
			createInstance();
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
		}),
		_geometries(ci.geometries)
	{
		// TODO Keep instance if possible?
		// We could only mark the BLAS for rebuild, rather than recreate its instance?
		for (size_t i = 0; i < _geometries.size(); ++i)
		{
			Callback cb{
				.callback = [this]() {
					destroyInstance();
				},
				.id = this,
			};
			Geometry & g = _geometries[i];
			g.vertex_buffer.buffer->addInvalidationCallback(cb);
			if (g.index_buffer && g.index_buffer.buffer != g.vertex_buffer.buffer)
			{
				g.index_buffer.buffer->addInvalidationCallback(cb);
			}
		}
	}

	BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure()
	{
		for (size_t i = 0; i < _geometries.size(); ++i)
		{
			Geometry& g = _geometries[i];
			g.vertex_buffer.buffer->removeInvalidationCallbacks(this);
			if (g.index_buffer && g.index_buffer.buffer != g.vertex_buffer.buffer)
			{
				g.index_buffer.buffer->removeInvalidationCallbacks(this);
			}
		}
	}






	TopLevelAccelerationStructureInstance::TopLevelAccelerationStructureInstance(CreateInfo const& ci) :
		AccelerationStructureInstance(AccelerationStructureInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
			.geometry_flags = ci.geometry_flags,
			.build_flags = ci.build_flags,
			.storage_buffer = ci.storage_buffer,
			.build_mode = ci.build_mode,
		}),
		_instances_buffer(ci.instances_buffer),
		_blases()
	{
		_max_primitive_count = ci.capacities;
		create();
	}

	void TopLevelAccelerationStructureInstance::write()
	{
		for (size_t i = 0; i < _blases.size(); ++i)
		{
			BLASInstance & b = _blases[i];
			VkAccelerationStructureInstanceKHR instance{
				.transform = b.xform,
				.instanceCustomIndex = b.instanceCustomIndex,
				.mask = b.mask,
				.instanceShaderBindingTableRecordOffset = b.instanceShaderBindingTableRecordOffset,
				.flags = b.flags,
				.accelerationStructureReference = b.blasi->address(),
			};
			//_instances_buffer.set<VkAccelerationStructureInstanceKHR>(i, instance);
		}
	}


	void TopLevelAccelerationStructureInstance::create()
	{
		_geometries.resize(1);
		_geometries[0] = VkAccelerationStructureGeometryKHR{
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
			.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
			.geometry = VkAccelerationStructureGeometryDataKHR{
				.instances = VkAccelerationStructureGeometryInstancesDataKHR{
					.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
					.pNext = nullptr,
					.arrayOfPointers = VK_FALSE,
					.data = 0, // later
				},
			},
			.flags = _geometry_flags,
		};

		uint32_t max_capacity = _instances_buffer.size() / sizeof(VkAccelerationStructureInstanceKHR);
		if (_max_primitive_count[0] > max_capacity)
		{
			_max_primitive_count[0] = max_capacity;
		}

		AccelerationStructureInstance::create();
	}

	TopLevelAccelerationStructureInstance::~TopLevelAccelerationStructureInstance()
	{

	}

	void TopLevelAccelerationStructureInstance::link()
	{
		_geometries[0].geometry.instances.data.deviceAddress = _instances_buffer.deviceAddress();
	}

	TopLevelAccelerationStructure::TopLevelAccelerationStructure(CreateInfo const& ci) :
		AccelerationStructure(AccelerationStructure::CI{
			.app = ci.app,
			.name = ci.name,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
			.geometry_flags = ci.geometry_flags,
			.build_flags = ci.build_flags,
			.storage_buffer = ci.storage_buffer,
		}),
		_instances_buffer(HostManagedBuffer::CI{
			.app = application(),
			.name = name() + "instances_buffer",
			.size = sizeof(VkAccelerationStructureInstanceKHR) * 16,
			.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		})
	{
		// register blas callback
		_instances_buffer.buffer()->addInvalidationCallback(Callback{
			.callback = [this]() {
				destroyInstance();
			},
			.id = this,
		});
	}

	TopLevelAccelerationStructure::~TopLevelAccelerationStructure()
	{
		// unregister blas callback
		_instances_buffer.buffer()->removeInvalidationCallbacks(this);

		for (size_t i = 0; i < _blases.size(); ++i)
		{
			_blases[i].blas->removeInvalidationCallbacks(this);
		}
	}

	void TopLevelAccelerationStructure::updateResources(UpdateContext& ctx)
	{
		if (_update_tick >= ctx.updateTick())
		{
			return;
		}

		_update_tick = ctx.updateTick();

		size_t compact_counter = 0;
		VkBuildAccelerationStructureModeKHR build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;
		for (size_t i = 0; i < _blases.size(); ++i)
		{
			BLASInstance & bi = _blases[i];
			if (bi.blas && bi.blas->instance())
			{
				bool write = false;
				if (compact_counter != bi._compact_id)
				{
					// Relocate
					bi._compact_id = compact_counter;
					write = true;
				}
				else if(bi._mark_for_update)
				{
					write = true;
				}
				if (write)
				{
					_instances_buffer.set<VkAccelerationStructureInstanceKHR>(compact_counter, VkAccelerationStructureInstanceKHR{
						.transform = bi.xform,
						.instanceCustomIndex = bi.instanceCustomIndex,
						.mask = bi.mask,
						.instanceShaderBindingTableRecordOffset = bi.instanceShaderBindingTableRecordOffset,
						.flags = bi.flags,
						.accelerationStructureReference = bi.blas->instance()->address(),
					});
					build_mode = std::min(build_mode, VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR);
				}
				++compact_counter;
			}
		}
		

		_instances_buffer.updateResources(ctx);
		
		if (!_inst)
		{
			createInstance();
		}

		if (_inst)
		{
			TLASI * inst = reinterpret_cast<TLASI*>(_inst.get());
			inst->requireBuildMode(build_mode);
			inst->setPrimitiveCountIFN(compact_counter);
			inst->link();
		}
	}


	void TopLevelAccelerationStructure::createInstance()
	{
		_inst = std::make_shared<TopLevelAccelerationStructureInstance>(TopLevelAccelerationStructureInstance::CI{
			.app = application(),
			.name = name(),
			.geometry_flags = _geometry_flags.value(),
			.build_flags = _build_flags,
			.capacities = {static_cast<uint32_t>(_instances_buffer.byteSize() / sizeof(VkAccelerationStructureInstanceKHR)), },
			.instances_buffer = _instances_buffer.fullBufferAndRangeInstance(),
			.storage_buffer = _storage_buffer.getInstance(),
			.build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		});
	}

	void TopLevelAccelerationStructure::recordTransferIFN(ExecutionRecorder& exec)
	{
		_instances_buffer.recordTransferIFN(exec);
	}

	void TopLevelAccelerationStructure::registerBLAS(uint32_t index, BLASInstance const& blas_instance)
	{
		if (_blases.size() <= index)
		{
			_blases.resize(index + 1);
		}
		const bool new_blas = _blases[index].blas != blas_instance.blas;
		if (new_blas && _blases[index].blas)
		{
			_blases[index].blas->removeInvalidationCallbacks(this);
		}
		_blases[index] = blas_instance;
		if (new_blas && _blases[index].blas)
		{
			_blases[index].blas->addInvalidationCallback(Callback{
				.callback = [this, index]() {
					_blases[index]._mark_for_update = true;
				},
				.id = this,
			});
		}
		if (_inst)
		{
			_inst->requireUpdate();
		}
	}
}