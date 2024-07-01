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
		_geometry_flags(ci.geometry_flags),
		_type(ci.type),
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
			.geometryCount = _vk_geometries.size32(),
			.pGeometries = _vk_geometries.data(),
			.ppGeometries = nullptr,
			.scratchData = 0,
		};

		_build_sizes = VkAccelerationStructureBuildSizesInfoKHR{
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
			.pNext = nullptr,
		};
		assert(_vk_geometries.size() == _max_primitive_count.size());
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
		InstanceHolder<AccelerationStructureInstance>(ci.app, ci.name, ci.hold_instance),
		_type(ci.type),
		_geometry_flags(ci.geometry_flags),
		_build_flags(ci.build_flags),
		_storage_buffer(ci.storage_buffer)
	{

	}

	AccelerationStructure::~AccelerationStructure()
	{
		
	}
}