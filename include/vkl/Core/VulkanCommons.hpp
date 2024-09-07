#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#define GLM_FORCE_LEFT_HANDED 1
//#define VK_USE_PLATFORM_WIN32_KHR

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <stdexcept>
#include <utility>
#include <string>
#include <mutex>
#include <cassert>
#include <vector>
#include <deque>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <concepts>
#include <filesystem>

#include <that/core/Range.hpp>
#include <that/stl_ext/alignment.hpp>
#include <that/utils/ExtensibleStringContainer.hpp>

#include "DynamicValue.hpp"

#include <vkl/Utils/MyVector.hpp>
#include <vkl/Utils/OptVector.hpp>
#include <vkl/Utils/stl_extension.hpp>

#define assertm(exp, msg) assert(((void)msg, exp))

#define VK_LOG std::cout << "[Vk]: " 
#define VK_ERROR_LOG std::cerr << "[Vk Error]: "

#define VKL_BREAKPOINT_HANDLE {int _ = 0;}
#define VKL_UNUSED(x) ((void)x)

#define VKL_BUILD_ANY_DEBUG (VKL_BUILD_DEBUG || VKL_BUILD_FAST_DEBUG)

#if VKL_BUILD_ANY_DEBUG

#define VK_CHECK(call, msg)				\
if (call != VK_SUCCESS) {				\
	assertm(false, msg);				\
}										

#else

#define VK_CHECK(call, msg)				\
call

#endif

#define NOT_YET_IMPLEMENTED assert(false)

#define UBO_ALIGNEMENT 16

#define ubo_vec2 alignas(UBO_ALIGNEMENT / 2) glm::vec2
#define ubo_vec3 alignas(UBO_ALIGNEMENT) glm::vec3
#define ubo_vec4 alignas(UBO_ALIGNEMENT) glm::vec4
#define ubo_mat4 alignas(UBO_ALIGNEMENT) glm::mat4 

template <class T, class Allocator>
struct std::ContainerUseCommonAppendConcatAndOperators<std::vector<T, Allocator>> : public std::true_type
{};

template <class T, class Allocator>
struct std::ContainerUseCommonAppendConcatAndOperators<std::deque<T, Allocator>> : public std::true_type
{};

template <class T, class Allocator>
struct std::ContainerUseCommonAppendConcatAndOperators<std::list<T, Allocator>> : public std::true_type
{};

template <class T, typename Less, class Allocator>
struct std::SetUseCommonOperators<std::set<T, Less, Allocator>> : public std::true_type
{};

template <class T, class H, class Eq, class Allocator>
struct std::SetUseCommonOperators<std::unordered_set<T, H, Eq, Allocator>> : public std::true_type
{};

namespace vkl
{
	class VkObject;

	struct VkStruct
	{
		VkStructureType sType;
		const void * pNext;
	};

	namespace concepts
	{
		template <typename S>
		concept VkStructLike = requires(S s)
		{
			{ s.sType } -> std::same_as<VkStructureType&>;
			{ s.pNext } -> std::convertible_to<const void*>;
			requires offsetof(S, sType) == offsetof(VkStruct, sType);
			requires offsetof(S, pNext) == offsetof(VkStruct, pNext);
		};
	}

	struct pNextChain
	{
		VkStruct * current = nullptr;

		template <concepts::VkStructLike VkStructLike>
		constexpr pNextChain(VkStructLike * vk_struct):
			current(reinterpret_cast<VkStruct*>(vk_struct))
		{}

		constexpr pNextChain(nullptr_t n = nullptr) {};

		const VkStruct* find(VkStructureType type) const
		{
			const VkStruct * res = current;
			while (res)
			{
				if (res->sType == type)
				{
					break;
				}
				res = reinterpret_cast<const VkStruct*>(res->pNext);
			}
			return res;
		}

		template <concepts::VkStructLike VkStructLike>
		constexpr pNextChain& link(VkStructLike* vk_struct)
		{
			if (current)
			{
				current->pNext = vk_struct;
			}
			if (vk_struct)
			{
				current = reinterpret_cast<VkStruct*>(vk_struct);
			}
			return *this;
		}

		template <concepts::VkStructLike VkStructLike>
		constexpr pNextChain& operator+=(VkStructLike* vk_struct)
		{
			return link(vk_struct);
		}

		constexpr pNextChain& link(nullptr_t)
		{
			if (current)
			{
				current->pNext = nullptr;
			}
			return *this;
		}

		constexpr pNextChain& operator+=(nullptr_t)
		{
			return link(nullptr);
		}
	};

	template <::std::concepts::Set<std::string_view> S>
	MyVector<const char*> flatten(S const& s)
	{
		MyVector<const char*> res(s.size());
		auto it = s.begin();
		for (size_t i = 0; i < res.size(); ++i)
		{
			res[i] = it->data();
			++it;
		}
		return res;
	}

	class VulkanExtensionsSet;

	template <class T>
	using Array = MyVector<T>;

	extern std::mutex g_mutex;
	
	struct Callback
	{
		std::function<void(void)> callback;
		const void * id = nullptr;
	};

	template <class Index>
	using Range = that::Range<Index>;
	using Range32u = that::Range32u;
	using Range64u = that::Range64u;
	using Range32i = that::Range32i;
	using Range64i = that::Range64i;
	using Range_st = that::Range_st;

	constexpr VkBufferUsageFlags VK_BUFFER_USAGE_TRANSFER_BITS = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	constexpr VkBufferUsageFlags2KHR VK_BUFFER_USAGE_2_TRANSFER_BITS_KHR = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT_KHR | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR;
	constexpr VkImageUsageFlags VK_IMAGE_USAGE_TRANSFER_BITS = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	constexpr VkImageAspectFlags VK_IMAGE_ASPECT_DEPTH_STENCIL_BITS = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

	constexpr VkColorComponentFlags VK_COLOR_COMPONENTS_RGB_BITS = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
	constexpr VkColorComponentFlags VK_COLOR_COMPONENTS_RGBA_BITS = VK_COLOR_COMPONENTS_RGB_BITS | VK_COLOR_COMPONENT_A_BIT;

	extern size_t getContiguousIndexFromVkFormat(VkFormat);
	extern VkFormat getVkFormatFromContiguousIndex(size_t);

	extern std::string getVkPresentModeKHRName(VkPresentModeKHR);
	extern std::string getVkColorSpaceKHRName(VkColorSpaceKHR);
	extern std::string getVkFormatName(VkFormat);
	extern bool checkVkFormatIsValid(VkFormat);
	extern VkImageAspectFlags getImageAspectFromFormat(VkFormat);


	
	enum class DrawType
	{
		None,
		Draw,
		Dispatch = Draw,
		DrawIndexed,
		IndirectDraw,
		IndirectDispatch = IndirectDraw,
		IndirectDrawIndexed,
		IndirectDrawCount,
		IndirectDrawCountIndexed,
		MultiDrawn,
		MultiDrawIndexed,
		MAX_ENUM,
	};
	using DispatchType = DrawType;
	
	extern uint32_t vkGetPhysicalDeviceAPIVersion(VkPhysicalDevice device);
	
	struct VulkanFeatures
	{
		VkPhysicalDeviceFeatures2 features2 = {};
		VkPhysicalDeviceVulkan11Features features_11 = {};
		VkPhysicalDeviceVulkan12Features features_12 = {};
		VkPhysicalDeviceVulkan13Features features_13 = {};

		VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchain_maintenance1_ext = {};
		VkPhysicalDevicePresentIdFeaturesKHR present_id_khr = {};
		VkPhysicalDevicePresentWaitFeaturesKHR present_wait_khr = {};

		VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shader_atomic_float_ext = {};
		VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shader_atomic_float_2_ext = {};

		VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragment_shading_rate_khr = {};
		VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT multisampled_render_to_single_sampled_ext = {};
		VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT subpass_merge_feedback = {};

		VkPhysicalDeviceLineRasterizationFeaturesEXT line_raster_ext = {};
		VkPhysicalDeviceIndexTypeUint8FeaturesEXT index_uint8_ext = {};
		VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_ext = {};
		VkPhysicalDeviceRobustness2FeaturesEXT robustness2_ext = {};

		
		VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_khr = {};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_khr = {};
		VkPhysicalDeviceRayQueryFeaturesKHR ray_query_khr = {};
		VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR ray_tracing_maintenance1_khr = {};
		VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR ray_tracing_position_fetch_khr = {};
		
		VkPhysicalDeviceRayTracingMotionBlurFeaturesNV ray_tracing_motion_blur_nv = {};
		VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV ray_tracing_invocation_reorder_nv = {};
		
		VkPhysicalDeviceRayTracingValidationFeaturesNV ray_tracing_validation_nv = {};

		VulkanFeatures();

		VkPhysicalDeviceFeatures2& link(uint32_t version, std::function<bool(std::string_view ext_name)> const& filter_extensions);

		VulkanFeatures operator&&(VulkanFeatures const& other) const;

		VulkanFeatures operator||(VulkanFeatures const& other) const;

		uint32_t count() const;
	};

	inline extern VulkanFeatures filterFeatures(VulkanFeatures const& requested, VulkanFeatures const& available)
	{
		return requested && available;
	}

	struct VulkanDeviceProps
	{
		VkPhysicalDeviceProperties2 props2 = {};
		VkPhysicalDeviceVulkan11Properties props_11 = {};
		VkPhysicalDeviceVulkan12Properties props_12 = {};
		VkPhysicalDeviceVulkan13Properties props_13 = {};

		VkPhysicalDeviceFragmentShadingRatePropertiesKHR fragment_shading_rate_khr = {};

		VkPhysicalDeviceLineRasterizationPropertiesEXT line_raster_ext = {};
		VkPhysicalDeviceMeshShaderPropertiesEXT mesh_shader_ext = {};
		VkPhysicalDeviceRobustness2PropertiesEXT robustness2_ext = {};

		VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_khr = {};
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_khr = {};

		VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV ray_tracing_invocation_reorder_nv = {};

		VulkanDeviceProps();

		VkPhysicalDeviceProperties2& link(uint32_t version, std::function<bool(std::string_view ext_name)> const& filter_extensions);
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

	enum class DescriptorSetName : uint32_t
	{
		common = 0,
		scene = 1,
		module = 2,
		shader = 3,
		invocation = 4,
		MAX_ENUM,
	};

	const static std::string s_descriptor_set_names[] = {
		"common",
		"scene",
		"module",
		"shader",
		"invocation"
	};

	struct BindingIndex
	{
		uint32_t set = 0;
		uint32_t binding = 0;

		constexpr BindingIndex operator+(uint32_t b) const
		{
			return BindingIndex{ .set = set, .binding = binding + b };
		}

		std::string asString()const;
	};

	struct DescriptorSetBindingGlobalOptions
	{
		bool use_push_descriptors = false;
		bool merge_module_and_shader = false;
		std::vector<BindingIndex> set_bindings = {};
		uint32_t shader_set = 0;
	};

	constexpr VkComponentMapping defaultComponentMapping()
	{
		VkComponentMapping res(VK_COMPONENT_SWIZZLE_IDENTITY);
		return res;
	}

	constexpr VkExtent2D makeUniformExtent2D(uint32_t n)
	{
		return VkExtent2D{
			.width = n, .height = n,
		};
	}

	constexpr VkExtent3D makeUniformExtent3D(uint32_t n)
	{
		return VkExtent3D{
			.width = n, .height = n, .depth = n,
		};
	}

	constexpr VkOffset2D makeUniformOffset2D(int32_t n)
	{
		return VkOffset2D{
			.x = n, .y = n,
		};
	}

	constexpr VkOffset3D makeUniformOffset3D(int32_t n)
	{
		return VkOffset3D{
			.x = n, .y = n, .z = n,
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

	inline VkExtent2D minimumExtent(VkExtent2D const& a, VkExtent2D const& b)
	{
		return VkExtent2D{
			.width = std::min(a.width, b.width),
			.height = std::min(a.height, b.height),
		};
	}

	inline VkExtent3D minimumExtent(VkExtent3D const& a, VkExtent3D const& b)
	{
		return VkExtent3D{
			.width = std::min(a.width, b.width),
			.height = std::min(a.height, b.height),
			.depth = std::min(a.depth, b.depth),
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

	constexpr VkImageSubresourceRange MakeZeroImageSubRange()
	{
		return VkImageSubresourceRange{
			.aspectMask = VK_IMAGE_ASPECT_NONE,
			.baseMipLevel = 0,
			.levelCount = 0,
			.baseArrayLayer = 0,
			.layerCount = 0,
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

	template <std::convertible_to<std::function<VkBool32(VkBool32, VkBool32)>> BinOp>
	void VkBool32ArrayOp(VkBool32 *res, const VkBool32* a, const VkBool32 * b, size_t n, BinOp const& op)
	{
		for (size_t i = 0; i < n; ++i)
		{
			res[i] = op(a[i], b[i]);
		}
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

		constexpr void clear()
		{
			_data.clear();
			_type = &typeid(void);
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
		ObjectView(std::vector<T> const& v)
		{
			_data = v.data();
			_size = v.size() * sizeof(T);
		}

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

		constexpr void clear()
		{
			_data = nullptr;
			_size = 0;
			_storage.clear();
		}
	};

	using PushConstant = ObjectView;


	struct PositionedObjectView
	{
		ObjectView obj = {};
		size_t pos = 0;
	};

	using DefinitionsList = that::ExtensibleStringContainer32;
}

template<class Stream>
Stream& operator<<(Stream& s, vkl::BindingIndex const& b)
{
	s << "(set = " << b.set << ", binding = " << b.binding << ")";
	return s;
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

constexpr bool operator==(VkSurfaceFormatKHR const& a, VkSurfaceFormatKHR const& b)
{
	return a.format == b.format && a.colorSpace == b.colorSpace;
}

constexpr bool operator!=(VkSurfaceFormatKHR const& a, VkSurfaceFormatKHR const& b)
{
	return !(a == b);
}

namespace std
{
	template<class K, class V, class Hasher = std::hash<K>, class Eq = std::equal_to<K>>
	using HMap = std::unordered_map<K, V, Hasher, Eq>;

	template<>
	struct hash<VkImageSubresourceRange>
	{
		size_t operator()(VkImageSubresourceRange const& r) const
		{
			const std::hash<size_t> hs;
			const std::hash<uint32_t> hu;
			// don't consider the aspect in the hash since it should be the same
			return hu(r.baseMipLevel) ^ hu(r.levelCount) ^ hu(r.baseArrayLayer) ^ hu(r.layerCount);
		}
	};

	template<class UInt>
	struct hash<vkl::Range<UInt>>
	{
		size_t operator()(vkl::Range<UInt> const& r) const noexcept
		{
			hash<UInt> h;
			hash<size_t> hs;
			return hs(h(r.begin) xor h(r.len));
		}
	};
}

namespace vkl
{
	using MountingPoints = std::HMap<std::string, std::string>;

	extern std::filesystem::path ReplaceMountingPoints(MountingPoints const& mp, std::filesystem::path const& p);
}