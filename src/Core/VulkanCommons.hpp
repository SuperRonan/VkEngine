#pragma once

//#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <stdexcept>
#include <utility>

#include "DynamicValue.hpp"

#define VK_LOG std::cout << "[Vk]: " 
#define VK_ERROR_LOG std::cerr << "[Vk Error]: "

#define VK_CHECK(call, msg)				\
if (call != VK_SUCCESS) {				\
	throw std::runtime_error(msg);		\
}										\

namespace vkl
{
	class VkObject;
	
	struct Callback
	{
		std::function<void(void)> callback;
		VkObject* id = nullptr;
	};

	template <class UInt>
	struct Range
	{
		UInt begin = 0;
		UInt len = 0;

		template <class Q>
		constexpr bool operator==(Range<Q> const& o) const
		{
			return begin == o.begin && len == o.len;
		}
	};

	using Range32 = Range<uint32_t>;
	using Range64 = Range<uint64_t>;
	using Range_st = Range<size_t>;


	static constexpr VkBufferUsageFlags VK_BUFFER_USAGE_TRANSFER_BITS = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	static constexpr VkImageUsageFlags VK_IMAGE_USAGE_TRANSFER_BITS = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;


	struct VulkanFeatures
	{
		VkPhysicalDeviceFeatures features = {};
		VkPhysicalDeviceVulkan11Features features_11 = {};
		VkPhysicalDeviceVulkan12Features features_12 = {};
		VkPhysicalDeviceVulkan13Features features_13 = {};
		VkPhysicalDeviceLineRasterizationFeaturesEXT line_raster_ext = {};

		VkPhysicalDeviceFeatures2 link()
		{
			features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
			features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			line_raster_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;

			features_11.pNext = &features_12;
			features_12.pNext = &features_13;
			features_13.pNext = &line_raster_ext;
			return VkPhysicalDeviceFeatures2{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
				.pNext = &features_11,
				.features = features,
			};
		}
	};

	struct VulkanDeviceProps
	{
		VkPhysicalDeviceProperties props = {};
		// TODO add new vulkan versions props
		VkPhysicalDeviceLineRasterizationPropertiesEXT line_raster_ext = {};

		VkPhysicalDeviceProperties2 link()
		{
			line_raster_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT;
			return VkPhysicalDeviceProperties2{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
				.pNext = &line_raster_ext,
				.properties = props,
			};
		}
	};

	struct VertexInputDescription
	{
		std::vector<VkVertexInputBindingDescription> binding;
		std::vector<VkVertexInputAttributeDescription> attrib;

		VkPipelineVertexInputStateCreateInfo link() const
		{
			return VkPipelineVertexInputStateCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.vertexBindingDescriptionCount = static_cast<uint32_t>(binding.size()),
				.pVertexBindingDescriptions = binding.data(),
				.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrib.size()),
				.pVertexAttributeDescriptions = attrib.data(),
			};
		}

		constexpr VertexInputDescription() {};

		constexpr VertexInputDescription(VertexInputDescription const&) = default;
		constexpr VertexInputDescription(VertexInputDescription &&) = default;

		constexpr VertexInputDescription& operator=(VertexInputDescription const&) = default;
		constexpr VertexInputDescription& operator=(VertexInputDescription &&) = default;

		VertexInputDescription(VkPipelineVertexInputStateCreateInfo const& ci)
		{
			binding.resize(ci.vertexBindingDescriptionCount);
			std::copy(ci.pVertexBindingDescriptions, ci.pVertexBindingDescriptions + ci.vertexBindingDescriptionCount, binding.data());

			attrib.resize(ci.vertexAttributeDescriptionCount);
			std::copy(ci.pVertexAttributeDescriptions, ci.pVertexAttributeDescriptions + ci.vertexAttributeDescriptionCount, attrib.data());
		}
	};


	constexpr VkComponentMapping defaultComponentMapping()
	{
		VkComponentMapping res(VK_COMPONENT_SWIZZLE_IDENTITY);
		return res;
	}

	constexpr VkExtent2D makeZeroExtent2D()
	{
		return VkExtent2D{
			.width = 0,
			.height = 0,
		};
	}

	constexpr VkExtent3D makeZeroExtent3D()
	{
		return VkExtent3D{
			.width = 0,
			.height = 0,
			.depth = 0,
		};
	}

	constexpr VkOffset2D makeZeroOffset2D()
	{
		return VkOffset2D{
			.x = 0,
			.y = 0,
		};
	}

	constexpr VkOffset3D makeZeroOffset3D()
	{
		return VkOffset3D{
			.x = 0,
			.y = 0,
			.z = 0,
		};
	}

	constexpr VkOffset3D convert(VkExtent3D const& ext)
	{
		return VkOffset3D{
			.x = (int32_t)ext.width,
			.y = (int32_t)ext.height,
			.z = (int32_t)ext.depth,
		};
	}

	constexpr VkExtent3D convert(VkOffset3D const& off)
	{
		return VkExtent3D{
			.width = (uint32_t)off.x,
			.height = (uint32_t)off.y,
			.depth = (uint32_t)off.z,
		};
	}

	constexpr VkExtent3D extend(VkExtent2D const& e2, uint32_t d = 1)
	{
		return VkExtent3D{
			.width = e2.width,
			.height = e2.height,
			.depth = d,
		};
	}

	constexpr VkOffset3D extend(VkOffset2D const& o2, int z = 0)
	{
		return VkOffset3D{
			.x = o2.x,
			.y = o2.y,
			.z = z,
		};
	}

	inline DynamicValue<VkExtent3D> extend(DynamicValue<VkExtent2D> const& e2, uint32_t d = 1)
	{
		return [=]() {
			return extend(e2.value(), d);
		};
	}

	inline DynamicValue<VkOffset3D> extend(DynamicValue<VkOffset2D> const& o2, int z = 0)
	{
		return [=]() {
			return extend(o2.value(), z);
		};
	}
	
	constexpr VkExtent2D extract(VkExtent3D e3)
	{
		return VkExtent2D{
			.width = e3.width,
			.height = e3.height,
		};
	}

	constexpr VkOffset2D extract(VkOffset3D o3)
	{
		return VkOffset2D{
			.x = o3.x,
			.y = o3.y,
		};
	}

	inline DynamicValue<VkExtent2D> extract(DynamicValue<VkExtent3D> const& e3)
	{
		return [=]()
		{
			return extract(*e3);
		};
	}

	inline DynamicValue<VkOffset2D> extract(DynamicValue<VkOffset3D> const& o3)
	{
		return [=]()
		{
			return extract(*o3);
		};
	}

	constexpr VkImageViewType getDefaultViewTypeFromImageType(VkImageType type)
	{
		switch (type)
		{
		case VK_IMAGE_TYPE_1D:
			return VK_IMAGE_VIEW_TYPE_1D;
			break;
		case VK_IMAGE_TYPE_2D:
			return VK_IMAGE_VIEW_TYPE_2D;
			break;
		case VK_IMAGE_TYPE_3D:
			return VK_IMAGE_VIEW_TYPE_3D;
			break;
		default:
			return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
			break;
		}
	}

	constexpr VkImageSubresourceLayers getImageLayersFromRange(VkImageSubresourceRange const& range)
	{
		return VkImageSubresourceLayers{
			.aspectMask = range.aspectMask,
			.mipLevel = range.baseMipLevel,
			.baseArrayLayer = range.baseArrayLayer,
			.layerCount = range.layerCount,
		};
	}

	constexpr VkPipelineStageFlags getPipelineStageFromShaderStage(VkShaderStageFlags s)
	{
		VkPipelineStageFlags res = 0;
		if (s & VK_SHADER_STAGE_VERTEX_BIT)						res |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		if (s & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)		res |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
		if (s & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)	res |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
		if (s & VK_SHADER_STAGE_GEOMETRY_BIT)					res |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
		if (s & VK_SHADER_STAGE_FRAGMENT_BIT)					res |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		if (s & VK_SHADER_STAGE_COMPUTE_BIT)					res |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		if (s & VK_SHADER_STAGE_RAYGEN_BIT_KHR)					res |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_ANY_HIT_BIT_KHR)				res |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)			res |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_MISS_BIT_KHR)					res |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_INTERSECTION_BIT_KHR)			res |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_CALLABLE_BIT_KHR)				res |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_TASK_BIT_NV)					res |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV;
		if (s & VK_SHADER_STAGE_MESH_BIT_NV)					res |= VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
		return res;
	}

	constexpr VkPipelineStageFlags2 getPipelineStageFromShaderStage2(VkShaderStageFlags s)
	{
		VkPipelineStageFlags res = 0;
		if (s & VK_SHADER_STAGE_VERTEX_BIT)						res |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
		if (s & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)		res |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT;
		if (s & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)	res |= VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;
		if (s & VK_SHADER_STAGE_GEOMETRY_BIT)					res |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
		if (s & VK_SHADER_STAGE_FRAGMENT_BIT)					res |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		if (s & VK_SHADER_STAGE_COMPUTE_BIT)					res |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		if (s & VK_SHADER_STAGE_RAYGEN_BIT_KHR)					res |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_ANY_HIT_BIT_KHR)				res |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)			res |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_MISS_BIT_KHR)					res |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_INTERSECTION_BIT_KHR)			res |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_CALLABLE_BIT_KHR)				res |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		if (s & VK_SHADER_STAGE_TASK_BIT_NV)					res |= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV;
		if (s & VK_SHADER_STAGE_MESH_BIT_NV)					res |= VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV;
		return res;
	}

	constexpr VkImageAspectFlags getImageAspectFromFormat(VkFormat f)
	{
		if (f >= VK_FORMAT_R4G4_UNORM_PACK8 && f <= VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)
		{
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
		else if (f >= VK_FORMAT_D16_UNORM && f <= VK_FORMAT_D32_SFLOAT)
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (f == VK_FORMAT_S8_UINT)
		{
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else if (f >= VK_FORMAT_D16_UNORM_S8_UINT && f <= VK_FORMAT_D32_SFLOAT_S8_UINT)
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			// TODO compressed, packed and multiplanar formats
			assert(false);
			// Probably a color aspect though
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	template <class Op>
	void VkBool32ArrayOp(VkBool32 *res, const VkBool32* a, const VkBool32 * b, size_t n, Op const& op)
	{
		for (size_t i = 0; i < n; ++i)
		{
			res[i] = op(a[i], b[i]);
		}
	}

	inline VulkanFeatures filterFeatures(VulkanFeatures const& requested, VulkanFeatures const& available)
	{
		const auto op_and = [](VkBool32 a, VkBool32 b) {return a & b; };

		VulkanFeatures res;
		
		VkBool32ArrayOp(
			&res.features.robustBufferAccess,
			&requested.features.robustBufferAccess,
			&available.features.robustBufferAccess,
			(offsetof(VkPhysicalDeviceFeatures, inheritedQueries) - offsetof(VkPhysicalDeviceFeatures, robustBufferAccess)) / sizeof(VkBool32) + 1, 
			op_and
		);

#define FILTER_VK_FEATURES(VK_VER, firstFeature, lastFeature) \
		VkBool32ArrayOp( \
			&res.features_##VK_VER . firstFeature, \
			&requested.features_##VK_VER . firstFeature, \
			&available.features_##VK_VER . firstFeature, \
			(offsetof(VkPhysicalDeviceVulkan##VK_VER##Features, lastFeature) - offsetof(VkPhysicalDeviceVulkan##VK_VER##Features, firstFeature)) / sizeof(VkBool32) + 1, \
			op_and \
		)

		FILTER_VK_FEATURES(11, storageBuffer16BitAccess, shaderDrawParameters);
		FILTER_VK_FEATURES(12, samplerMirrorClampToEdge, subgroupBroadcastDynamicId);
		FILTER_VK_FEATURES(13, robustImageAccess, maintenance4);

#undef FILTER_VK_FEATURES

		VkBool32ArrayOp(
			&res.line_raster_ext.rectangularLines,
			&requested.line_raster_ext.rectangularLines,
			&available.line_raster_ext.rectangularLines,
			(offsetof(VkPhysicalDeviceLineRasterizationFeaturesEXT, stippledSmoothLines) - offsetof(VkPhysicalDeviceLineRasterizationFeaturesEXT, rectangularLines)) / sizeof(VkBool32) + 1,
			op_and
		);

		return res;
	}

	namespace vk_operators
	{
		
	}

	class AnyPOD
	{
	protected:

		std::vector<uint8_t> _data = {};
		const std::type_info * _type = &typeid(void);

	public:

		constexpr AnyPOD() = default;

		template <class T>
		AnyPOD(T const& t) :
			_type(&typeid(T))
		{
			_data.resize(sizeof(T));
			std::memcpy(_data.data(), &t, sizeof(T));
		}

		constexpr AnyPOD(std::type_info const& rti) :
			_type(&rti)
		{}

		constexpr AnyPOD(AnyPOD const& other) :
			_data(other._data),
			_type(other._type)
		{}

		constexpr AnyPOD(AnyPOD&& other) noexcept:
			_data(std::move(other._data)),
			_type(other._type)
		{
			other._data.clear();
			other._type = &typeid(void);
		}

		constexpr AnyPOD& operator=(AnyPOD const& other)
		{
			_data = other._data;
			_type = other._type;
			return *this;
		}

		constexpr AnyPOD& operator=(AnyPOD&& other) noexcept
		{
			std::swap(_data, other._data);
			std::swap(_type, other._type);
			return *this;
		}

		constexpr const void* data()const
		{
			return _data.data();
		}

		constexpr size_t size()const
		{
			return _data.size();
		}

		const std::type_info& type()const
		{
			assert(_type != nullptr);
			return *_type;
		}

		constexpr bool empty() const
		{
			return _data.empty();
		}

		constexpr bool hasValue() const
		{
			return !empty();
		}
	};


	class ObjectView
	{
	protected:

		const void* _data = nullptr;
		size_t _size = 0;
		AnyPOD _storage = {};

	public:

		constexpr ObjectView() = default;

		constexpr ObjectView(const void* ptr, size_t size) :
			_data(ptr),
			_size(size)
		{}

		template <class T>
		ObjectView(T const& t)
		{
			if constexpr (std::is_pointer<T>::value)
			{
				using Q = typename std::remove_pointer<T>::type;
				_storage = AnyPOD(typeid(Q));
				_data = t;
				_size = sizeof(Q);
			}
			else
			{
				_storage = AnyPOD(t);
				_data = _storage.data();
				_size = _storage.size();
			}
		}

		constexpr ObjectView(ObjectView const& other) :
			_storage(other._storage)
		{
			if (_storage.empty())
			{
				_data = other._data;
				_size = other._size;
			}
			else
			{
				_data = _storage.data();
				_size = _storage.size();
			}
		}

		constexpr ObjectView(ObjectView && other) noexcept:
			_storage(std::move(other._storage))
		{
			if (_storage.empty())
			{
				std::swap(_data, other._data);
				std::swap(_size, other._size);
			}
			else
			{
				_data = _storage.data();
				_size = _storage.size();
			}
		}

		constexpr ObjectView& operator=(ObjectView const& other)
		{
			_storage = other._storage;
			if (_storage.empty())
			{
				_data = other._data;
				_size = other._size;
			}
			else
			{
				_data = _storage.data();
				_size = _storage.size();
			}
			return *this;
		}

		constexpr ObjectView& operator=(ObjectView && other) noexcept
		{
			_storage = std::move(other._storage);
			if (_storage.empty())
			{
				std::swap(_data, other._data);
				std::swap(_size, other._size);
			}
			else
			{
				_data = _storage.data();
				_size = _storage.size();
			}
			return *this;
		}

		constexpr const void* data()const
		{
			return _data;
		}

		constexpr size_t size()const
		{
			return _size;
		}

		constexpr uint32_t size32()const
		{
			return static_cast<uint32_t>(size());
		}

		const std::type_info& type()const
		{
			return _storage.type();
		}

		constexpr bool empty()const
		{
			return _data == nullptr;
		}

		constexpr bool hasValue()const
		{
			return !empty();
		}

		constexpr bool ownsValue()const
		{
			return _storage.hasValue();
		}
	};
}


constexpr bool operator==(VkImageSubresourceRange const& a, VkImageSubresourceRange const& b)
{
	return (a.aspectMask == b.aspectMask)
		&& (a.baseArrayLayer == b.baseArrayLayer)
		&& (a.baseMipLevel == b.baseMipLevel)
		&& (a.layerCount == b.layerCount)
		&& (a.levelCount == b.levelCount);
}

constexpr bool operator==(VkExtent3D const& a, VkExtent3D const& b)
{
	return (a.width == b.width) && (a.height == b.height) && (a.depth == b.depth);
}

constexpr bool operator!=(VkExtent3D const& a, VkExtent3D const& b)
{
	return !(a == b);
}

constexpr bool operator==(VkExtent2D const& a, VkExtent2D const& b)
{
	return (a.width == b.width) && (a.height == b.height);
}

constexpr bool operator!=(VkExtent2D const& a, VkExtent2D const& b)
{
	return !(a == b);
}