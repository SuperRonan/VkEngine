#include <vkl/Core/VulkanCommons.hpp>
#include <unordered_map>
#include <sstream>


namespace vkl
{
	std::mutex g_common_mutex = {};


	using namespace std::string_literals;
	
	uint32_t vkGetPhysicalDeviceAPIVersion(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(device, &props);
		return props.apiVersion;
	}

	VulkanFeatures::VulkanFeatures()
	{
		std::memset(this, 0, sizeof(VulkanFeatures));
	}

	VkPhysicalDeviceFeatures2& VulkanFeatures::link(uint32_t version, std::function<bool(std::string_view ext_name)> const& filter_extensions)
	{
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

		swapchain_maintenance1_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
		present_id_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
		present_wait_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;

		shader_atomic_float_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
		shader_atomic_float_2_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;

		fragment_shading_rate_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
		multisampled_render_to_single_sampled_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_FEATURES_EXT;
		subpass_merge_feedback.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_MERGE_FEEDBACK_FEATURES_EXT;

		line_raster_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
		index_uint8_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT;
		mesh_shader_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
		robustness2_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;

		fragment_shader_barycentric_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;

		acceleration_structure_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		ray_tracing_pipeline_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		ray_query_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		ray_tracing_maintenance1_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR;
		ray_tracing_position_fetch_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR;

		ray_tracing_motion_blur_nv.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV;
		ray_tracing_invocation_reorder_nv.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_FEATURES_NV;

		ray_tracing_validation_nv.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_VALIDATION_FEATURES_NV;

		compute_shader_derivative_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR;

		pNextChain chain = &features2;

		if(version >= VK_MAKE_VERSION(1, 1, 0))
			chain += &features_11;
		if (version >= VK_MAKE_VERSION(1, 2, 0))
			chain += &features_12;
		if (version >= VK_MAKE_VERSION(1, 3, 0))
			chain += &features_13;

		if(filter_extensions(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME))
			chain += &swapchain_maintenance1_ext;
		if(filter_extensions(VK_KHR_PRESENT_ID_EXTENSION_NAME))
			chain += &present_id_khr;
		if(filter_extensions(VK_KHR_PRESENT_WAIT_EXTENSION_NAME))
			chain += &present_wait_khr;

		if(filter_extensions(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME))
			chain += &shader_atomic_float_ext;
		if(filter_extensions(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME))
			chain += &shader_atomic_float_2_ext;
		if(filter_extensions(VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME))
			chain += &shader_image_atomic_int64_ext;

		if(filter_extensions(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
			chain += &fragment_shading_rate_khr;
		if(filter_extensions(VK_EXT_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_EXTENSION_NAME))
			chain += &multisampled_render_to_single_sampled_ext;
		if(filter_extensions(VK_EXT_SUBPASS_MERGE_FEEDBACK_EXTENSION_NAME))
			chain += &subpass_merge_feedback;

		if(filter_extensions(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME))
			chain += &line_raster_ext;
		if(filter_extensions(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME))
			chain += &index_uint8_ext;
		if(filter_extensions(VK_EXT_MESH_SHADER_EXTENSION_NAME))
			chain += &mesh_shader_ext;
		if(filter_extensions(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME))
			chain += &robustness2_ext;

		if(filter_extensions(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME))
			chain += &fragment_shader_barycentric_khr;

		if(filter_extensions(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
			chain += &acceleration_structure_khr;
		if(filter_extensions(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
			chain += &ray_tracing_pipeline_khr;
		if(filter_extensions(VK_KHR_RAY_QUERY_EXTENSION_NAME))
			chain += &ray_query_khr;
		if(filter_extensions(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME))
			chain += &ray_tracing_maintenance1_khr;
		if(filter_extensions(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME))
			chain += &ray_tracing_position_fetch_khr;

		if(filter_extensions(VK_NV_RAY_TRACING_MOTION_BLUR_EXTENSION_NAME))
			chain += &ray_tracing_motion_blur_nv;
		if(filter_extensions(VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME))
			chain += &ray_tracing_invocation_reorder_nv;
		
		if(filter_extensions(VK_NV_RAY_TRACING_VALIDATION_EXTENSION_NAME))
			chain += &ray_tracing_validation_nv;

		if(filter_extensions(VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME))
			chain += &compute_shader_derivative_khr;

		chain += nullptr;
		return features2;
	}

	VulkanDeviceProps::VulkanDeviceProps()
	{
		std::memset(this, 0, sizeof(VulkanDeviceProps));
	}

	VkPhysicalDeviceProperties2& VulkanDeviceProps::link(uint32_t version, std::function<bool(std::string_view ext_name)> const& filter_extensions)
	{
		props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		props_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
		props_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
		props_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;

		fragment_shading_rate_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;

		line_raster_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT;
		mesh_shader_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
		robustness2_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT;

		fragment_shader_barycentric_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR;

		acceleration_structure_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
		ray_tracing_pipeline_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

		ray_tracing_invocation_reorder_nv.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_PROPERTIES_NV;

		compute_shader_derivative_khr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_PROPERTIES_KHR;

		pNextChain chain = &props2;

		if (version >= VK_MAKE_VERSION(1, 1, 0))
			chain += &props_11;
		if (version >= VK_MAKE_VERSION(1, 2, 0))
			chain += &props_12;
		if (version >= VK_MAKE_VERSION(1, 3, 0))
			chain += &props_13;
		
		if(filter_extensions(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
			chain += &fragment_shading_rate_khr;

		if (filter_extensions(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME))
			chain += &line_raster_ext;
		if (filter_extensions(VK_EXT_MESH_SHADER_EXTENSION_NAME))
			chain += &mesh_shader_ext;
		if(filter_extensions(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME))
			chain += &robustness2_ext;

		if (filter_extensions(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME))
			chain += &fragment_shader_barycentric_khr;

		if (filter_extensions(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
			chain += &acceleration_structure_khr;
		if (filter_extensions(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
			chain += &ray_tracing_pipeline_khr;

		if (filter_extensions(VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME))
			chain += &ray_tracing_invocation_reorder_nv;

		if (filter_extensions(VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME))
			chain += &compute_shader_derivative_khr;

		chain += nullptr;
		return props2;
	}
	
	template <std::convertible_to<std::function<VkBool32(VkBool32, VkBool32)>> BinOp>
	VulkanFeatures combineFeatures(VulkanFeatures const& requested, VulkanFeatures const& available, BinOp const& op)
	{
		VulkanFeatures res;

		VkBool32ArrayOp(
			&res.features2.features.robustBufferAccess,
			&requested.features2.features.robustBufferAccess,
			&available.features2.features.robustBufferAccess,
			(offsetof(VkPhysicalDeviceFeatures, inheritedQueries) - offsetof(VkPhysicalDeviceFeatures, robustBufferAccess)) / sizeof(VkBool32) + 1,
			op
		);

#define COMBINE_VK_FEATURES(StructName, variableName, firstFeature, lastFeature) \
		VkBool32ArrayOp( \
			&res. variableName . firstFeature, \
			&requested. variableName . firstFeature, \
			&available. variableName . firstFeature, \
			(offsetof(StructName, lastFeature) - offsetof(StructName, firstFeature)) / sizeof(VkBool32) + 1, \
			op \
		)

#define COMBINE_VK_FEATURES_STD(VK_VER, firstFeature, lastFeature) COMBINE_VK_FEATURES(VkPhysicalDeviceVulkan##VK_VER##Features, features_##VK_VER, firstFeature, lastFeature)

		COMBINE_VK_FEATURES_STD(11, storageBuffer16BitAccess, shaderDrawParameters);
		COMBINE_VK_FEATURES_STD(12, samplerMirrorClampToEdge, subgroupBroadcastDynamicId);
		COMBINE_VK_FEATURES_STD(13, robustImageAccess, maintenance4);


		//COMBINE_VK_FEATURES(VkPhysicalDevice, , , );

		COMBINE_VK_FEATURES(VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT, swapchain_maintenance1_ext, swapchainMaintenance1, swapchainMaintenance1);
		COMBINE_VK_FEATURES(VkPhysicalDevicePresentIdFeaturesKHR, present_id_khr, presentId, presentId);
		COMBINE_VK_FEATURES(VkPhysicalDevicePresentWaitFeaturesKHR, present_wait_khr, presentWait, presentWait);

		COMBINE_VK_FEATURES(VkPhysicalDeviceShaderAtomicFloatFeaturesEXT, shader_atomic_float_ext, shaderBufferFloat32Atomics, sparseImageFloat32AtomicAdd);
		COMBINE_VK_FEATURES(VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT, shader_atomic_float_2_ext, shaderBufferFloat16Atomics, sparseImageFloat32AtomicMinMax);
		COMBINE_VK_FEATURES(VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT, shader_image_atomic_int64_ext, shaderImageInt64Atomics, sparseImageInt64Atomics);

		COMBINE_VK_FEATURES(VkPhysicalDeviceFragmentShadingRateFeaturesKHR, fragment_shading_rate_khr, pipelineFragmentShadingRate, attachmentFragmentShadingRate);
		COMBINE_VK_FEATURES(VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT, multisampled_render_to_single_sampled_ext, multisampledRenderToSingleSampled, multisampledRenderToSingleSampled);
		COMBINE_VK_FEATURES(VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT, subpass_merge_feedback, subpassMergeFeedback, subpassMergeFeedback);

		COMBINE_VK_FEATURES(VkPhysicalDeviceLineRasterizationFeaturesEXT, line_raster_ext, rectangularLines, stippledSmoothLines);
		COMBINE_VK_FEATURES(VkPhysicalDeviceIndexTypeUint8FeaturesEXT, index_uint8_ext, indexTypeUint8, indexTypeUint8);
		COMBINE_VK_FEATURES(VkPhysicalDeviceMeshShaderFeaturesEXT, mesh_shader_ext, taskShader, meshShaderQueries);
		COMBINE_VK_FEATURES(VkPhysicalDeviceRobustness2FeaturesEXT, robustness2_ext, robustBufferAccess2, nullDescriptor);
		
		COMBINE_VK_FEATURES(VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR, fragment_shader_barycentric_khr, fragmentShaderBarycentric, fragmentShaderBarycentric);

		COMBINE_VK_FEATURES(VkPhysicalDeviceAccelerationStructureFeaturesKHR, acceleration_structure_khr, accelerationStructure, descriptorBindingAccelerationStructureUpdateAfterBind);
		COMBINE_VK_FEATURES(VkPhysicalDeviceRayTracingPipelineFeaturesKHR, ray_tracing_pipeline_khr, rayTracingPipeline, rayTraversalPrimitiveCulling);
		COMBINE_VK_FEATURES(VkPhysicalDeviceRayQueryFeaturesKHR, ray_query_khr, rayQuery, rayQuery);
		COMBINE_VK_FEATURES(VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR, ray_tracing_maintenance1_khr, rayTracingMaintenance1, rayTracingPipelineTraceRaysIndirect2);
		COMBINE_VK_FEATURES(VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR, ray_tracing_position_fetch_khr, rayTracingPositionFetch, rayTracingPositionFetch);

		COMBINE_VK_FEATURES(VkPhysicalDeviceRayTracingMotionBlurFeaturesNV, ray_tracing_motion_blur_nv, rayTracingMotionBlur, rayTracingMotionBlurPipelineTraceRaysIndirect);
		COMBINE_VK_FEATURES(VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV, ray_tracing_invocation_reorder_nv, rayTracingInvocationReorder, rayTracingInvocationReorder);
		
		COMBINE_VK_FEATURES(VkPhysicalDeviceRayTracingValidationFeaturesNV, ray_tracing_validation_nv, rayTracingValidation, rayTracingValidation);

		COMBINE_VK_FEATURES(VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR, compute_shader_derivative_khr, computeDerivativeGroupQuads, computeDerivativeGroupLinear);

#undef COMBINE_VK_FEATURES_STD
#undef COMBINE_VK_FEATURES

		return res;
	}
	
	VulkanFeatures VulkanFeatures::operator&&(VulkanFeatures const& other)const
	{
		return combineFeatures(*this, other, [](VkBool32 a, VkBool32 b){return a & b;});
	}

	VulkanFeatures VulkanFeatures::operator||(VulkanFeatures const& other)const
	{
		return combineFeatures(*this, other, [](VkBool32 a, VkBool32 b) {return a | b; });
	}

	uint32_t VulkanFeatures::count()const
	{
		uint32_t res = 0;

		auto count_f = [](const VkBool32* begin, uint32_t len)
		{
			uint32_t res = 0;
			for (uint32_t i = 0; i < len; ++i)
			{
				if(begin[i])	++res;
			}
			return res;
		};

		res += count_f(
			&this->features2.features.robustBufferAccess, 
			(offsetof(VkPhysicalDeviceFeatures, inheritedQueries) - offsetof(VkPhysicalDeviceFeatures, robustBufferAccess)) / sizeof(VkBool32) + 1
		);

#define COUNT_VK_FEATURES(StructName, variableName, firstFeature, lastFeature) \
		count_f( \
			&this-> variableName . firstFeature, \
			(offsetof(StructName, lastFeature) - offsetof(StructName, firstFeature)) / sizeof(VkBool32) + 1 \
		)

#define COUNT_VK_FEATURES_STD(VK_VER, firstFeature, lastFeature) COUNT_VK_FEATURES(VkPhysicalDeviceVulkan##VK_VER##Features, features_##VK_VER, firstFeature, lastFeature) 

		res += COUNT_VK_FEATURES_STD(11, storageBuffer16BitAccess, shaderDrawParameters);
		res += COUNT_VK_FEATURES_STD(12, samplerMirrorClampToEdge, subgroupBroadcastDynamicId);
		res += COUNT_VK_FEATURES_STD(13, robustImageAccess, maintenance4);

		//res += COUNT_VK_FEATURES(VkPhysicaldevice, , , );
		
		res += COUNT_VK_FEATURES(VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT, swapchain_maintenance1_ext, swapchainMaintenance1, swapchainMaintenance1);
		res += COUNT_VK_FEATURES(VkPhysicalDevicePresentIdFeaturesKHR, present_id_khr, presentId, presentId);
		res += COUNT_VK_FEATURES(VkPhysicalDevicePresentWaitFeaturesKHR, present_wait_khr, presentWait, presentWait);

		res += COUNT_VK_FEATURES(VkPhysicalDeviceShaderAtomicFloatFeaturesEXT, shader_atomic_float_ext, shaderBufferFloat32Atomics, sparseImageFloat32AtomicAdd);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT, shader_atomic_float_2_ext, shaderBufferFloat16Atomics, sparseImageFloat32AtomicMinMax);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT, shader_image_atomic_int64_ext, shaderImageInt64Atomics, sparseImageInt64Atomics);

		res += COUNT_VK_FEATURES(VkPhysicalDeviceFragmentShadingRateFeaturesKHR, fragment_shading_rate_khr, pipelineFragmentShadingRate, attachmentFragmentShadingRate);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT, multisampled_render_to_single_sampled_ext, multisampledRenderToSingleSampled, multisampledRenderToSingleSampled);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT, subpass_merge_feedback, subpassMergeFeedback, subpassMergeFeedback);

		res += COUNT_VK_FEATURES(VkPhysicalDeviceLineRasterizationFeaturesEXT, line_raster_ext, rectangularLines, stippledSmoothLines);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceIndexTypeUint8FeaturesEXT, index_uint8_ext, indexTypeUint8, indexTypeUint8);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceMeshShaderFeaturesEXT, mesh_shader_ext, taskShader, meshShaderQueries);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceRobustness2FeaturesEXT, robustness2_ext, robustBufferAccess2, nullDescriptor);

		res += COUNT_VK_FEATURES(VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR, fragment_shader_barycentric_khr, fragmentShaderBarycentric, fragmentShaderBarycentric);
		
		res += COUNT_VK_FEATURES(VkPhysicalDeviceAccelerationStructureFeaturesKHR, acceleration_structure_khr, accelerationStructure, descriptorBindingAccelerationStructureUpdateAfterBind);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceRayTracingPipelineFeaturesKHR, ray_tracing_pipeline_khr, rayTracingPipeline, rayTraversalPrimitiveCulling);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceRayQueryFeaturesKHR, ray_query_khr, rayQuery, rayQuery);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR, ray_tracing_maintenance1_khr, rayTracingMaintenance1, rayTracingPipelineTraceRaysIndirect2);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR, ray_tracing_position_fetch_khr, rayTracingPositionFetch, rayTracingPositionFetch);
		
		res += COUNT_VK_FEATURES(VkPhysicalDeviceRayTracingMotionBlurFeaturesNV, ray_tracing_motion_blur_nv, rayTracingMotionBlur, rayTracingMotionBlurPipelineTraceRaysIndirect);
		res += COUNT_VK_FEATURES(VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV, ray_tracing_invocation_reorder_nv, rayTracingInvocationReorder, rayTracingInvocationReorder);
		
		res += COUNT_VK_FEATURES(VkPhysicalDeviceRayTracingValidationFeaturesNV, ray_tracing_validation_nv, rayTracingValidation, rayTracingValidation);

		res += COUNT_VK_FEATURES(VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR, compute_shader_derivative_khr, computeDerivativeGroupQuads, computeDerivativeGroupLinear);

#undef COUNT_VK_FEATURES_STD
#undef COUNT_VK_FEATURES


		return res;
	}


	std::string BindingIndex::asString() const
	{
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}


	std::string getVkPresentModeKHRName(VkPresentModeKHR p)
	{
		std::string res;
		switch (p)
		{
			case VK_PRESENT_MODE_IMMEDIATE_KHR :						res = "Immediate"; break;
			case VK_PRESENT_MODE_MAILBOX_KHR :							res = "Mailbox"; break;
			case VK_PRESENT_MODE_FIFO_KHR :								res = "FIFO"; break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR :						res = "FIFO Relaxed"; break;
			case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR :			res = "Shared Demand Refresh"; break;
			case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR :		res = "Shader Continuous Refresh"; break;
		}
		return res;
	}

	std::string getVkColorSpaceKHRName(VkColorSpaceKHR c)
	{
		std::string res;
		switch (c)
		{
			case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:						res = "sRGB NonLinear"; break;
			case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:				res = "Display P3 NonLinear"; break;
			case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:				res = "Extended sRGB Linear"; break;
			case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:					res = "Display P3 Linear"; break;
			case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:					res = "DCI P3 NonLinear"; break;
			case VK_COLOR_SPACE_BT709_LINEAR_EXT:						res = "BT709 Linear"; break;
			case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:					res = "BT709 NonLinear"; break;
			case VK_COLOR_SPACE_BT2020_LINEAR_EXT:						res = "BT2020 Linear"; break;
			case VK_COLOR_SPACE_HDR10_ST2084_EXT:						res = "HDR10 ST2084"; break;
			case VK_COLOR_SPACE_DOLBYVISION_EXT:						res = "Dolby Vision"; break;
			case VK_COLOR_SPACE_HDR10_HLG_EXT:							res = "HDR10 HLG"; break;
			case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:					res = "Adobe RGB Linear"; break;
			case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:					res = "Adobe RGB NonLinear"; break;
			case VK_COLOR_SPACE_PASS_THROUGH_EXT:						res = "Pass Through"; break;
			case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:			res = "Extended sRGB NonLinear"; break;
			case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:						res = "Display Native"; break;
		}
		return res;
	}

	static const std::vector<VkFormat> vk_format_contiguous_list = {
		VK_FORMAT_G8B8G8R8_422_UNORM,
		VK_FORMAT_B8G8R8G8_422_UNORM,
		VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
		VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
		VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
		VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,
		VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
		VK_FORMAT_R10X6_UNORM_PACK16,
		VK_FORMAT_R10X6G10X6_UNORM_2PACK16,
		VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
		VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
		VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
		VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
		VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
		VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
		VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
		VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
		VK_FORMAT_R12X4_UNORM_PACK16,
		VK_FORMAT_R12X4G12X4_UNORM_2PACK16,
		VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
		VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
		VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
		VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
		VK_FORMAT_G16B16G16R16_422_UNORM,
		VK_FORMAT_B16G16R16G16_422_UNORM,
		VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
		VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,
		VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
		VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,
		VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,
		VK_FORMAT_G8_B8R8_2PLANE_444_UNORM,
		VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16,
		VK_FORMAT_G16_B16R16_2PLANE_444_UNORM,
		VK_FORMAT_A4R4G4B4_UNORM_PACK16,
		VK_FORMAT_A4B4G4R4_UNORM_PACK16,
		VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK,
		VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG,
		VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG,
		VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG,
		VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG,
		VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,
		VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,
		VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,
		VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,
		VK_FORMAT_R16G16_S10_5_NV,
		VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR,
		VK_FORMAT_A8_UNORM_KHR,
		VK_FORMAT_MAX_ENUM,
	};

	static const std::vector<const char*> vk_format_names = {
		"UNDEFINED",
		"R4G4_UNORM_PACK8",
		"R4G4B4A4_UNORM_PACK16",
		"B4G4R4A4_UNORM_PACK16",
		"R5G6B5_UNORM_PACK16",
		"B5G6R5_UNORM_PACK16",
		"R5G5B5A1_UNORM_PACK16",
		"B5G5R5A1_UNORM_PACK16",
		"A1R5G5B5_UNORM_PACK16",
		"R8_UNORM",
		"R8_SNORM",
		"R8_USCALED",
		"R8_SSCALED",
		"R8_UINT",
		"R8_SINT",
		"R8_SRGB",
		"R8G8_UNORM",
		"R8G8_SNORM",
		"R8G8_USCALED",
		"R8G8_SSCALED",
		"R8G8_UINT",
		"R8G8_SINT",
		"R8G8_SRGB",
		"R8G8B8_UNORM",
		"R8G8B8_SNORM",
		"R8G8B8_USCALED",
		"R8G8B8_SSCALED",
		"R8G8B8_UINT",
		"R8G8B8_SINT",
		"R8G8B8_SRGB",
		"B8G8R8_UNORM",
		"B8G8R8_SNORM",
		"B8G8R8_USCALED",
		"B8G8R8_SSCALED",
		"B8G8R8_UINT",
		"B8G8R8_SINT",
		"B8G8R8_SRGB",
		"R8G8B8A8_UNORM",
		"R8G8B8A8_SNORM",
		"R8G8B8A8_USCALED",
		"R8G8B8A8_SSCALED",
		"R8G8B8A8_UINT",
		"R8G8B8A8_SINT",
		"R8G8B8A8_SRGB",
		"B8G8R8A8_UNORM",
		"B8G8R8A8_SNORM",
		"B8G8R8A8_USCALED",
		"B8G8R8A8_SSCALED",
		"B8G8R8A8_UINT",
		"B8G8R8A8_SINT",
		"B8G8R8A8_SRGB",
		"A8B8G8R8_UNORM_PACK32",
		"A8B8G8R8_SNORM_PACK32",
		"A8B8G8R8_USCALED_PACK32",
		"A8B8G8R8_SSCALED_PACK32",
		"A8B8G8R8_UINT_PACK32",
		"A8B8G8R8_SINT_PACK32",
		"A8B8G8R8_SRGB_PACK32",
		"A2R10G10B10_UNORM_PACK32",
		"A2R10G10B10_SNORM_PACK32",
		"A2R10G10B10_USCALED_PACK32",
		"A2R10G10B10_SSCALED_PACK32",
		"A2R10G10B10_UINT_PACK32",
		"A2R10G10B10_SINT_PACK32",
		"A2B10G10R10_UNORM_PACK32",
		"A2B10G10R10_SNORM_PACK32",
		"A2B10G10R10_USCALED_PACK32",
		"A2B10G10R10_SSCALED_PACK32",
		"A2B10G10R10_UINT_PACK32",
		"A2B10G10R10_SINT_PACK32",
		"R16_UNORM",
		"R16_SNORM",
		"R16_USCALED",
		"R16_SSCALED",
		"R16_UINT",
		"R16_SINT",
		"R16_SFLOAT",
		"R16G16_UNORM",
		"R16G16_SNORM",
		"R16G16_USCALED",
		"R16G16_SSCALED",
		"R16G16_UINT",
		"R16G16_SINT",
		"R16G16_SFLOAT",
		"R16G16B16_UNORM",
		"R16G16B16_SNORM",
		"R16G16B16_USCALED",
		"R16G16B16_SSCALED",
		"R16G16B16_UINT",
		"R16G16B16_SINT",
		"R16G16B16_SFLOAT",
		"R16G16B16A16_UNORM",
		"R16G16B16A16_SNORM",
		"R16G16B16A16_USCALED",
		"R16G16B16A16_SSCALED",
		"R16G16B16A16_UINT",
		"R16G16B16A16_SINT",
		"R16G16B16A16_SFLOAT",
		"R32_UINT",
		"R32_SINT",
		"R32_SFLOAT",
		"R32G32_UINT",
		"R32G32_SINT",
		"R32G32_SFLOAT",
		"R32G32B32_UINT",
		"R32G32B32_SINT",
		"R32G32B32_SFLOAT",
		"R32G32B32A32_UINT",
		"R32G32B32A32_SINT",
		"R32G32B32A32_SFLOAT",
		"R64_UINT",
		"R64_SINT",
		"R64_SFLOAT",
		"R64G64_UINT",
		"R64G64_SINT",
		"R64G64_SFLOAT",
		"R64G64B64_UINT",
		"R64G64B64_SINT",
		"R64G64B64_SFLOAT",
		"R64G64B64A64_UINT",
		"R64G64B64A64_SINT",
		"R64G64B64A64_SFLOAT",
		"B10G11R11_UFLOAT_PACK32",
		"E5B9G9R9_UFLOAT_PACK32",
		"D16_UNORM",
		"X8_D24_UNORM_PACK32",
		"D32_SFLOAT",
		"S8_UINT",
		"D16_UNORM_S8_UINT",
		"D24_UNORM_S8_UINT",
		"D32_SFLOAT_S8_UINT",
		"BC1_RGB_UNORM_BLOCK",
		"BC1_RGB_SRGB_BLOCK",
		"BC1_RGBA_UNORM_BLOCK",
		"BC1_RGBA_SRGB_BLOCK",
		"BC2_UNORM_BLOCK",
		"BC2_SRGB_BLOCK",
		"BC3_UNORM_BLOCK",
		"BC3_SRGB_BLOCK",
		"BC4_UNORM_BLOCK",
		"BC4_SNORM_BLOCK",
		"BC5_UNORM_BLOCK",
		"BC5_SNORM_BLOCK",
		"BC6H_UFLOAT_BLOCK",
		"BC6H_SFLOAT_BLOCK",
		"BC7_UNORM_BLOCK",
		"BC7_SRGB_BLOCK",
		"ETC2_R8G8B8_UNORM_BLOCK",
		"ETC2_R8G8B8_SRGB_BLOCK",
		"ETC2_R8G8B8A1_UNORM_BLOCK",
		"ETC2_R8G8B8A1_SRGB_BLOCK",
		"ETC2_R8G8B8A8_UNORM_BLOCK",
		"ETC2_R8G8B8A8_SRGB_BLOCK",
		"EAC_R11_UNORM_BLOCK",
		"EAC_R11_SNORM_BLOCK",
		"EAC_R11G11_UNORM_BLOCK",
		"EAC_R11G11_SNORM_BLOCK",
		"ASTC_4x4_UNORM_BLOCK",
		"ASTC_4x4_SRGB_BLOCK",
		"ASTC_5x4_UNORM_BLOCK",
		"ASTC_5x4_SRGB_BLOCK",
		"ASTC_5x5_UNORM_BLOCK",
		"ASTC_5x5_SRGB_BLOCK",
		"ASTC_6x5_UNORM_BLOCK",
		"ASTC_6x5_SRGB_BLOCK",
		"ASTC_6x6_UNORM_BLOCK",
		"ASTC_6x6_SRGB_BLOCK",
		"ASTC_8x5_UNORM_BLOCK",
		"ASTC_8x5_SRGB_BLOCK",
		"ASTC_8x6_UNORM_BLOCK",
		"ASTC_8x6_SRGB_BLOCK",
		"ASTC_8x8_UNORM_BLOCK",
		"ASTC_8x8_SRGB_BLOCK",
		"ASTC_10x5_UNORM_BLOCK",
		"ASTC_10x5_SRGB_BLOCK",
		"ASTC_10x6_UNORM_BLOCK",
		"ASTC_10x6_SRGB_BLOCK",
		"ASTC_10x8_UNORM_BLOCK",
		"ASTC_10x8_SRGB_BLOCK",
		"ASTC_10x10_UNORM_BLOCK",
		"ASTC_10x10_SRGB_BLOCK",
		"ASTC_12x10_UNORM_BLOCK",
		"ASTC_12x10_SRGB_BLOCK",
		"ASTC_12x12_UNORM_BLOCK",
		"ASTC_12x12_SRGB_BLOCK",
		"G8B8G8R8_422_UNORM",
		"B8G8R8G8_422_UNORM",
		"G8_B8_R8_3PLANE_420_UNORM",
		"G8_B8R8_2PLANE_420_UNORM",
		"G8_B8_R8_3PLANE_422_UNORM",
		"G8_B8R8_2PLANE_422_UNORM",
		"G8_B8_R8_3PLANE_444_UNORM",
		"R10X6_UNORM_PACK16",
		"R10X6G10X6_UNORM_2PACK16",
		"R10X6G10X6B10X6A10X6_UNORM_4PACK16",
		"G10X6B10X6G10X6R10X6_422_UNORM_4PACK16",
		"B10X6G10X6R10X6G10X6_422_UNORM_4PACK16",
		"G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16",
		"G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16",
		"G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16",
		"G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16",
		"G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16",
		"R12X4_UNORM_PACK16",
		"R12X4G12X4_UNORM_2PACK16",
		"R12X4G12X4B12X4A12X4_UNORM_4PACK16",
		"G12X4B12X4G12X4R12X4_422_UNORM_4PACK16",
		"B12X4G12X4R12X4G12X4_422_UNORM_4PACK16",
		"G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16",
		"G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16",
		"G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16",
		"G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16",
		"G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16",
		"G16B16G16R16_422_UNORM",
		"B16G16R16G16_422_UNORM",
		"G16_B16_R16_3PLANE_420_UNORM",
		"G16_B16R16_2PLANE_420_UNORM",
		"G16_B16_R16_3PLANE_422_UNORM",
		"G16_B16R16_2PLANE_422_UNORM",
		"G16_B16_R16_3PLANE_444_UNORM",
		"MAX_ENUM",
	};

	static const size_t vk_format_last_contiguous_format = size_t(VK_FORMAT_ASTC_12x12_SRGB_BLOCK);

	static const std::unordered_map<VkFormat, size_t> vk_format_contiguous_map = {
		{VK_FORMAT_G8B8G8R8_422_UNORM, 0},
		{VK_FORMAT_B8G8R8G8_422_UNORM, 1},
		{VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM, 2},
		{VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, 3},
		{VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM, 4},
		{VK_FORMAT_G8_B8R8_2PLANE_422_UNORM, 5},
		{VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM, 6},
		{VK_FORMAT_R10X6_UNORM_PACK16, 7},
		{VK_FORMAT_R10X6G10X6_UNORM_2PACK16, 8},
		{VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16, 9},
		{VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16, 10},
		{VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16, 11},
		{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, 12},
		{VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, 13},
		{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, 14},
		{VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16, 15},
		{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, 16},
		{VK_FORMAT_R12X4_UNORM_PACK16, 17},
		{VK_FORMAT_R12X4G12X4_UNORM_2PACK16, 18},
		{VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16, 19},
		{VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16, 20},
		{VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16, 21},
		{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, 22},
		{VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16, 23},
		{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16, 24},
		{VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16, 25},
		{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16, 26},
		{VK_FORMAT_G16B16G16R16_422_UNORM, 27},
		{VK_FORMAT_B16G16R16G16_422_UNORM, 28},
		{VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM, 29},
		{VK_FORMAT_G16_B16R16_2PLANE_420_UNORM, 30},
		{VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM, 31},
		{VK_FORMAT_G16_B16R16_2PLANE_422_UNORM, 32},
		{VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM, 33},
		{VK_FORMAT_G8_B8R8_2PLANE_444_UNORM, 34},
		{VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16, 35},
		{VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16, 36},
		{VK_FORMAT_G16_B16R16_2PLANE_444_UNORM, 37},
		{VK_FORMAT_A4R4G4B4_UNORM_PACK16, 38},
		{VK_FORMAT_A4B4G4R4_UNORM_PACK16, 39},
		{VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK, 40},
		{VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK, 41},
		{VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK, 42},
		{VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK, 43},
		{VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK, 44},
		{VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK, 45},
		{VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK, 46},
		{VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK, 47},
		{VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK, 48},
		{VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK, 49},
		{VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK, 50},
		{VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK, 51},
		{VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK, 52},
		{VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK, 53},
		{VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, 54},
		{VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, 55},
		{VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, 56},
		{VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, 57},
		{VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG, 58},
		{VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG, 59},
		{VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG, 60},
		{VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG, 61},
		{VK_FORMAT_R16G16_S10_5_NV, 62},
		{VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR, 63},
		{VK_FORMAT_A8_UNORM_KHR, 64},
		{VK_FORMAT_MAX_ENUM, 65},
	};
	
	
	size_t getContiguousIndexFromVkFormat(VkFormat f)
	{
		size_t res = size_t(f);
		if (res > vk_format_last_contiguous_format)
		{
			res = vk_format_contiguous_map.at(f) + (vk_format_last_contiguous_format + 1);
		}
		return res;
	}

	VkFormat getVkFormatFromContiguousIndex(size_t i)
	{
		VkFormat res = VK_FORMAT_UNDEFINED;
		if (i <= vk_format_last_contiguous_format)
		{
			res = static_cast<VkFormat>(i);
		}
		else
		{
			res = vk_format_contiguous_list.at(i - (vk_format_last_contiguous_format + 1));
		}
		return res;
	}

	std::string getVkFormatName(VkFormat f)
	{
		return vk_format_names.at(getContiguousIndexFromVkFormat(f));
	}

	bool checkVkFormatIsValid(VkFormat f)
	{
		bool res = true;
		size_t i = size_t(f);
		if (i > vk_format_last_contiguous_format)
		{
			res = vk_format_contiguous_map.contains(f);
		}
		return res;
	}

	VkImageAspectFlags getImageAspectFromFormat(VkFormat f)
	{
		// The detailed format could take care of this
		VkImageAspectFlags res = 0;
		
		if (f >= VK_FORMAT_R4G4_UNORM_PACK8 && f <= VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)
		{
			res = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		else if (f >= VK_FORMAT_D16_UNORM && f <= VK_FORMAT_D32_SFLOAT_S8_UINT)
		{
			switch (f)
			{
				case VK_FORMAT_D16_UNORM:
					res = VK_IMAGE_ASPECT_DEPTH_BIT;
					break;
				case VK_FORMAT_X8_D24_UNORM_PACK32:
					res = VK_IMAGE_ASPECT_DEPTH_BIT;
					break;
				case VK_FORMAT_D32_SFLOAT:
					res = VK_IMAGE_ASPECT_DEPTH_BIT;
					break;
				case VK_FORMAT_S8_UINT:
					res = VK_IMAGE_ASPECT_STENCIL_BIT;
					break;
				case VK_FORMAT_D16_UNORM_S8_UINT:
					res = VK_IMAGE_ASPECT_DEPTH_STENCIL_BITS;
					break;
				case VK_FORMAT_D24_UNORM_S8_UINT:
					res = VK_IMAGE_ASPECT_DEPTH_STENCIL_BITS;
					break;
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
					res = VK_IMAGE_ASPECT_DEPTH_BIT;
					break;
			}
		}
		else
		{
			assert(false);
		}
		
		return res;
	}
}