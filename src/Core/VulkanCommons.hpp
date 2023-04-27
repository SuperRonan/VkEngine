#pragma once

//#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <stdexcept>

#include "DynamicValue.hpp"

#define VK_LOG std::cout << "[Vk]: " 
#define VK_ERROR_LOG std::cerr << "[Vk Error]: "

#define VK_CHECK(call, msg)				\
if (call != VK_SUCCESS) {				\
	throw std::runtime_error(msg);		\
}										\

namespace vkl
{

	VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mips);

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

	inline DynamicValue<VkExtent3D> extendD(DynamicValue<VkExtent2D> const& e2, uint32_t d = 1)
	{
		return [=]() {
			return extend(e2.value(), d);
		};
	}

	inline DynamicValue<VkOffset3D> extendD(DynamicValue<VkOffset2D> const& o2, int z = 0)
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

	constexpr VkOffset2D extract(VkOffset2D o2)
	{
		return VkOffset2D{
			.x = o2.x,
			.y = o2.y,
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

	namespace vk_operators
	{
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

	}
}