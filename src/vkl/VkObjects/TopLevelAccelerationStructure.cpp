#include <vkl/VkObjects/TopLevelAccelerationStructure.hpp>

namespace vkl
{
	TopLevelAccelerationStructureInstance::TopLevelAccelerationStructureInstance(CreateInfo const& ci) :
		AccelerationStructureInstance(AccelerationStructureInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.geometry_flags = ci.geometry_flags,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
			.build_flags = ci.build_flags,
			.storage_buffer = ci.storage_buffer,
			.build_mode = ci.build_mode,
		}),
		_geometries(ci.geometries.size())
	{
		_max_primitive_count.resize(_geometries.size());
		//std::cout << "----------------------------------------------------" << std::endl;
		//std::cout << "Create TLAS with " << _geometries.size() << " geometries: " << std::endl;
		for (size_t i = 0; i < _geometries.size(); ++i)
		{
			Geometry & g = _geometries[i];
			g.flags = ci.geometries[i].flags;
			g.primitive_count = ci.geometries[i].capacity;
			g.instances_buffer = ci.geometries[i].instances_buffer;
			_max_primitive_count[i] = ci.geometries[i].capacity;
			//std::cout << "\t" << i << ": With a capacity of " << _max_primitive_count[i] << std::endl;
		}
		

		create();
	}

	void TopLevelAccelerationStructureInstance::write()
	{
		for (size_t i = 0; i < _geometries.size(); ++i)
		{
			Geometry& g = _geometries[i];
			for (size_t j = 0; j < g.blases.size(); ++j)
			{
				BLASInstance& b = g.blases[j];
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
	}


	void TopLevelAccelerationStructureInstance::create()
	{
		_vk_geometries.resize(_geometries.size());
		for (size_t i = 0; i < _geometries.size(); ++i)
		{
			Geometry& g = _geometries[i];
			_vk_geometries[i] = VkAccelerationStructureGeometryKHR{
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
				.flags = g.flags,
			};

			const uint32_t max_capacity = _geometries[i].instances_buffer.size() / sizeof(VkAccelerationStructureInstanceKHR);
			if (_max_primitive_count[i] > max_capacity)
			{
				_max_primitive_count[i] = max_capacity;
			}
		}

		AccelerationStructureInstance::create();
	}

	TopLevelAccelerationStructureInstance::~TopLevelAccelerationStructureInstance()
	{

	}

	void TopLevelAccelerationStructureInstance::link()
	{
		for (size_t i = 0; i < _vk_geometries.size(); ++i)
		{
			_vk_geometries[i].geometry.instances.data.deviceAddress = _geometries[i].instances_buffer.deviceAddress();
		}
	}

	void TopLevelAccelerationStructureInstance::setPrimitiveCountIFN(uint32_t geometry_id, uint32_t n)
	{
		assert(geometry_id < _geometries.size32());
		Geometry & g = _geometries[geometry_id];
		if (g.primitive_count != n)
		{
			requireRebuild();
			g.primitive_count = n;
			assert(_max_primitive_count[geometry_id] >= n);
		}
	}


	TopLevelAccelerationStructure::TopLevelAccelerationStructure(CreateInfo const& ci) :
		AccelerationStructure(AccelerationStructure::CI{
			.app = ci.app,
			.name = ci.name,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
			.geometry_flags = ci.geometry_flags,
			.build_flags = ci.build_flags,
			.storage_buffer = ci.storage_buffer,
			.hold_instance = ci.hold_instance,
		})
	{
		_geometries.resize(ci.geometries.size());
		for (size_t i = 0; i < _geometries.size(); ++i)
		{
			Geometry & g = _geometries[i];
			g.flags = ci.geometries[i].flags;
			g.blases.resize(ci.geometries[i].capacity);
			g.instances_buffer = std::make_shared<HostManagedBuffer>(HostManagedBuffer::CI{
				.app = application(),
				.name = name() + ".instances_buffer_"s + std::to_string(i),
				.size = sizeof(VkAccelerationStructureInstanceKHR) * ci.geometries[i].capacity,
				.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			});
			g.instances_buffer->buffer()->setInvalidationCallback(Callback{
				.callback = [this]() {
					destroyInstanceIFN();
				},
				.id = this,
			});
		}
	}

	TopLevelAccelerationStructure::~TopLevelAccelerationStructure()
	{
		for (size_t i = 0; i < _geometries.size(); ++i)
		{
			_geometries[i].instances_buffer->buffer()->removeInvalidationCallback(this);
		}
	}

	void TopLevelAccelerationStructure::updateResources(UpdateContext& ctx)
	{
		if (_update_tick >= ctx.updateTick())
		{
			return;
		}
		_update_tick = ctx.updateTick();

		static thread_local MyVector<uint32_t> _compact_counter;
		_compact_counter.resize(_geometries.size());

		if (checkHoldInstance())
		{
			VkBuildAccelerationStructureModeKHR build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;
			const VkGeometryFlagsKHR common_flags = _geometry_flags.valueOr(0);
			
			if (_inst)
			{
				if (_inst->geometryFlags() != common_flags)
				{
					_inst->requireRebuild();
				}
			}

			for (size_t i = 0; i < _geometries.size(); ++i)
			{
				uint32_t & compact_counter = _compact_counter[i];
				compact_counter = 0;

				Geometry& g = _geometries[i];
				for (size_t j = 0; j < g.blases.size(); ++j)
				{
					BLASInstance& bi = g.blases[j];
					if (bi.blas && bi.blas->instance())
					{
						bool write = false;
						if (bi.blas->instance() != bi._blas_instance)
						{
							bi._blas_instance = bi.blas->instance();
							write = true;
						}
						if (compact_counter != bi._compact_id)
						{
							// Relocate
							bi._compact_id = compact_counter;
							write = true;
						}
						else if (bi._mark_for_update)
						{
							write = true;
						}
						if (write)
						{
							g.instances_buffer->set<VkAccelerationStructureInstanceKHR>(compact_counter, VkAccelerationStructureInstanceKHR{
								.transform = bi.xform,
								.instanceCustomIndex = bi.instanceCustomIndex,
								.mask = bi.mask,
								.instanceShaderBindingTableRecordOffset = bi.instanceShaderBindingTableRecordOffset,
								.flags = bi.flags,
								.accelerationStructureReference = bi._blas_instance->address(),
								});
							build_mode = std::min(build_mode, VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR);
						}
						bi._mark_for_update = false;
						++compact_counter;
					}
				}
				g.instances_buffer->updateResources(ctx);
			}

			if (!_inst)
			{
				createInstance();
			}

			if (_inst)
			{
				TLASI* inst = static_cast<TLASI*>(_inst.get());
				inst->requireBuildMode(build_mode);
				for (size_t i = 0; i < _geometries.size(); ++i)
				{
					inst->setPrimitiveCountIFN(i, _compact_counter[i]);
				}
				inst->link();
			}
		}
	}


	void TopLevelAccelerationStructure::createInstance()
	{
		const VkGeometryFlagsKHR common_geom_flags = _geometry_flags.valueOr(0);
		const size_t n = _geometries.size();
		MyVector<TopLevelAccelerationStructureInstance::GeometryCreateInfo> geometries_ci(n);
		for (size_t i = 0; i < n; ++i)
		{
			TopLevelAccelerationStructureInstance::GeometryCreateInfo & gci = geometries_ci[i];
			Geometry & g = _geometries[i];
			gci.flags = g.flags.valueOr(0) | common_geom_flags;
			gci.instances_buffer = g.instances_buffer->getSegmentInstance();
			gci.capacity = gci.instances_buffer.size() / sizeof(VkAccelerationStructureInstanceKHR);
		}
		_inst = std::make_shared<TopLevelAccelerationStructureInstance>(TopLevelAccelerationStructureInstance::CI{
			.app = application(),
			.name = name(),
			.geometry_flags = common_geom_flags,
			.build_flags = _build_flags,
			.geometries = std::move(geometries_ci),
			.storage_buffer = _storage_buffer.getInstance(),
			.build_mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		});
	}

	void TopLevelAccelerationStructure::recordTransferIFN(ExecutionRecorder& exec)
	{
		for (size_t i = 0; i < _geometries.size(); ++i)
		{
			_geometries[i].instances_buffer->recordTransferIFN(exec);
		}
	}

	void TopLevelAccelerationStructure::registerBLAS(uint32_t geometry_index, uint32_t index, BLASInstance const& blas_instance)
	{
		assert(geometry_index < _geometries.size32());
		Geometry & g = _geometries[geometry_index];

		if (g.blases.size() <= index)
		{
			g.blases.resize(index + 1);
		}
		const bool new_blas = g.blases[index].blas != blas_instance.blas;
		//if (new_blas && _blases[index].blas)
		//{
		//	_blases[index].blas->removeInvalidationCallback(this);
		//}
		g.blases[index] = blas_instance;
		//if (new_blas && _blases[index].blas)
		//{
		//	_blases[index].blas->setInvalidationCallback(Callback{
		//		.callback = [this, index]() {
		//			_blases[index]._mark_for_update = true;
		//		},
		//		.id = this,
		//	});
		//}
		if (_inst)
		{
			_inst->requireUpdate();
		}
	}
}