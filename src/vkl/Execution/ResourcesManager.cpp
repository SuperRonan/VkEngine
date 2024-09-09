#include <vkl/Execution/ResourcesManager.hpp>

#include <ShaderLib/Vulkan/ShaderAtomicFlags.h>

namespace vkl
{

	ResourcesManager::ResourcesManager(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_shader_check_period(ci.shader_check_period),
		_common_definitions(std::make_unique<DefinitionsMap>()),
		_mounting_points(std::make_unique<MountingPoints>()),
		_upload_queue(UploadQueue::CI{
			.app = application(),
			.name = name() + ".uploadQueue",
		}),
		_mips_queue(MipMapComputeQueue::CI{
			.app = application(),
			.name = name() + ".MipMapQueue",
		}),
		_descriptor_writer(DescriptorWriter::CI{
			.app = application(),
			.name = name() + ".DescriptorWriter",
		})
	{
		_last_shader_check = _shader_clock_t::now() - 2 * _shader_check_period;

		populateCommonObjects();
	}

	void ResourcesManager::populateCommonObjects()
	{
		// Copy common mounting points
		for (auto it : application()->mountingPoints())
		{
			(*_mounting_points).insert(it);
		}

		VulkanFeatures const& features = application()->availableFeatures();
		VulkanDeviceProps const& props = application()->deviceProperties();

		const auto tod = [](VkBool32 b) {
			return b ? "1" : "0";
		};
		

		_common_definitions->setDefinition("SHADER_DEVICE_TYPE", std::to_string(props.props2.properties.deviceType));
		_common_definitions->setDefinition("SHADER_DEVICE_ID", std::to_string(props.props2.properties.deviceID));
		_common_definitions->setDefinition("SHADER_VENDOR_ID", std::to_string(props.props2.properties.vendorID));
		_common_definitions->setDefinition("SHADER_VK_API_VERSION", std::to_string(props.props2.properties.apiVersion));
		_common_definitions->setDefinition("SHADER_DRIVER_VERSION", std::to_string(props.props2.properties.driverVersion));
		_common_definitions->setDefinition("SHADER_DRIVER_ID", std::to_string(props.props_12.driverID));
		
		_common_definitions->setDefinition("SHADER_MAX_BOUND_DESCRIPTOR_SETS", std::to_string(props.props2.properties.limits.maxBoundDescriptorSets));
		_common_definitions->setDefinition("SHADER_MAX_PUSH_CONSTANT_SIZE", std::to_string(props.props2.properties.limits.maxPushConstantsSize));

		_common_definitions->setDefinition("SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT", tod(features.features2.features.shaderStorageImageReadWithoutFormat));
		_common_definitions->setDefinition("SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT", tod(features.features2.features.shaderStorageImageWriteWithoutFormat));
		_common_definitions->setDefinition("SHADER_IMAGE_GATHER_EXTENDED", tod(features.features2.features.shaderImageGatherExtended));
		_common_definitions->setDefinition("SHADER_STORAGE_IMAGE_EXTENDED_FORMATS", tod(features.features2.features.shaderStorageImageExtendedFormats));
		_common_definitions->setDefinition("SHADER_STORAGE_IMAGE_MULTISAMPLE", tod(features.features2.features.shaderStorageImageMultisample));

		_common_definitions->setDefinition("SHADER_CLIP_DISTANCE_AVAILABLE", tod(features.features2.features.shaderClipDistance));
		_common_definitions->setDefinition("SHADER_CULL_DISTANCE_AVAILABLE", tod(features.features2.features.shaderCullDistance));
		_common_definitions->setDefinition("SHADER_FP64_AVAILABLE", tod(features.features2.features.shaderFloat64));
		_common_definitions->setDefinition("SHADER_INT64_AVAILABLE", tod(features.features2.features.shaderInt64));
		_common_definitions->setDefinition("SHADER_INT16_AVAILABLE", tod(features.features2.features.shaderInt16));

		_common_definitions->setDefinition("SHADER_POINT_SIZE_RANGE_MIN", std::to_string(props.props2.properties.limits.pointSizeRange[0]));
		_common_definitions->setDefinition("SHADER_POINT_SIZE_RANGE_MAX", std::to_string(props.props2.properties.limits.pointSizeRange[1]));
		_common_definitions->setDefinition("SHADER_POINT_SIZE_GRANULARITY", std::to_string(props.props2.properties.limits.pointSizeGranularity));
		_common_definitions->setDefinition("SHADER_LINE_WIDTH_RANGE_MIN", std::to_string(props.props2.properties.limits.lineWidthRange[0]));
		_common_definitions->setDefinition("SHADER_LINE_WIDTH_RANGE_MAX", std::to_string(props.props2.properties.limits.lineWidthRange[1]));
		_common_definitions->setDefinition("SHADER_LINE_WIDTH_GRANULARITY", std::to_string(props.props2.properties.limits.lineWidthGranularity));

		_common_definitions->setDefinition("SHADER_MAX_FRAGMENT_OUTPUT_ATTACHMENTS", std::to_string(props.props2.properties.limits.maxFragmentOutputAttachments));

		_common_definitions->setDefinition("SHADER_GEOMETRY_AVAILABLE", tod(features.features2.features.geometryShader));
		_common_definitions->setDefinition("SHADER_MAX_GEOMETRY_INPUT_COMPONENTS", std::to_string(props.props2.properties.limits.maxGeometryInputComponents));
		_common_definitions->setDefinition("SHADER_MAX_GEOMETRY_OUTPUT_COMPONENTS", std::to_string(props.props2.properties.limits.maxGeometryOutputComponents));
		_common_definitions->setDefinition("SHADER_MAX_GEOMETRY_OUTPUT_VERTICES", std::to_string(props.props2.properties.limits.maxGeometryOutputVertices));
		_common_definitions->setDefinition("SHADER_MAX_GEOMETRY_SHADER_INVOCATIONS", std::to_string(props.props2.properties.limits.maxGeometryShaderInvocations));
		_common_definitions->setDefinition("SHADER_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS", std::to_string(props.props2.properties.limits.maxGeometryTotalOutputComponents));

		_common_definitions->setDefinition("SHADER_TESSELATION_AVAILABLE", tod(features.features2.features.tessellationShader));

		_common_definitions->setDefinition("SHADER_FP16_AVAILABLE", tod(features.features_12.shaderFloat16));
		_common_definitions->setDefinition("SHADER_SSBO_16BITS_ACCESS", tod(features.features_11.storageBuffer16BitAccess));

		_common_definitions->setDefinition("SHADER_MAX_COMPUTE_WORKGROUP_SUBGROUPS", std::to_string(props.props_13.maxComputeWorkgroupSubgroups));
		_common_definitions->setDefinition("SHADER_MAX_COMPUTE_LOCAL_SIZE", std::to_string(props.props2.properties.limits.maxComputeWorkGroupInvocations));
		_common_definitions->setDefinition("SHADER_MAX_COMPUTE_LOCAL_SIZE_X", std::to_string(props.props2.properties.limits.maxComputeWorkGroupSize[0]));
		_common_definitions->setDefinition("SHADER_MAX_COMPUTE_LOCAL_SIZE_Y", std::to_string(props.props2.properties.limits.maxComputeWorkGroupSize[1]));
		_common_definitions->setDefinition("SHADER_MAX_COMPUTE_LOCAL_SIZE_Z", std::to_string(props.props2.properties.limits.maxComputeWorkGroupSize[2]));

		_common_definitions->setDefinition("SHADER_RAY_QUERY_AVAILABLE", tod(features.ray_query_khr.rayQuery));
		_common_definitions->setDefinition("SHADER_RAY_TRACING_AVAILABLE", tod(features.ray_tracing_pipeline_khr.rayTracingPipeline));
		_common_definitions->setDefinition("SHADER_RAY_TRACING_MAX_RAY_HIT_ATTRIBUTE_SIZE", std::to_string(props.ray_tracing_pipeline_khr.maxRayHitAttributeSize));
		_common_definitions->setDefinition("SHADER_RAY_TRACING_MAX_RAY_RECURSION_DEPTH", std::to_string(props.ray_tracing_pipeline_khr.maxRayRecursionDepth));
		_common_definitions->setDefinition("SHADER_RAY_TRACING_INVOCATION_REORDER_AVAILABLE", tod(features.ray_tracing_invocation_reorder_nv.rayTracingInvocationReorder));
		_common_definitions->setDefinition("SHADER_RAY_TRACING_INVOCATION_REORDER_REORDERING_HINT", std::to_string(props.ray_tracing_invocation_reorder_nv.rayTracingInvocationReorderReorderingHint));
		_common_definitions->setDefinition("SHADER_RAY_TRACING_POSITION_FETCH_AVAILABLE", tod(features.ray_tracing_position_fetch_khr.rayTracingPositionFetch));

		_common_definitions->setDefinition("SHADER_SUBGROUP_SIZE", std::to_string(props.props_11.subgroupSize));
		_common_definitions->setDefinition("SHADER_SUBGROUP_SUPPORTED_OPERATIONS", std::to_string(props.props_11.subgroupSupportedOperations));
		_common_definitions->setDefinition("SHADER_SUBGROUP_SUPPORTED_STAGES", std::to_string(props.props_11.subgroupSupportedStages));

		_common_definitions->setDefinition("SHADER_MAX_TASK_LOCAL_SIZE", std::to_string(props.mesh_shader_ext.maxTaskWorkGroupInvocations));
		_common_definitions->setDefinition("SHADER_MAX_PREFERED_TASK_LOCAL_SIZE", std::to_string(props.mesh_shader_ext.maxPreferredTaskWorkGroupInvocations));
		_common_definitions->setDefinition("SHADER_MAX_MESH_LOCAL_SIZE", std::to_string(props.mesh_shader_ext.maxMeshWorkGroupInvocations));
		_common_definitions->setDefinition("SHADER_MAX_PREFERED_MESH_LOCAL_SIZE", std::to_string(props.mesh_shader_ext.maxPreferredMeshWorkGroupInvocations));
		_common_definitions->setDefinition("SHADER_MESH_OUTPUT_PER_PRIMITIVE_GRANULARITY", std::to_string(props.mesh_shader_ext.meshOutputPerPrimitiveGranularity));
		_common_definitions->setDefinition("SHADER_MESH_OUTPUT_PER_VERTEX_GRANULARITY", std::to_string(props.mesh_shader_ext.meshOutputPerVertexGranularity));
		_common_definitions->setDefinition("SHADER_MESH_PREFERS_COMPACT_PRIMITIVE_OUTPUT", tod(props.mesh_shader_ext.prefersCompactPrimitiveOutput));
		_common_definitions->setDefinition("SHADER_MESH_PREFERS_COMPACT_VERTEX_OUTPUT", tod(props.mesh_shader_ext.prefersCompactVertexOutput));
		_common_definitions->setDefinition("SHADER_MESH_PREFERS_LOCAL_INVOCATION_PRIMITIVE_OUTPUT", tod(props.mesh_shader_ext.prefersLocalInvocationPrimitiveOutput));
		_common_definitions->setDefinition("SHADER_MESH_PREFERS_LOCAL_INVOCATION_VERTEX_OUTPUT", tod(props.mesh_shader_ext.prefersLocalInvocationVertexOutput));


		_common_definitions->setDefinition("SHADER_GEOMETRY_PROCESSING_STAGES_STORES_AND_ATOMICS", tod(features.features2.features.vertexPipelineStoresAndAtomics));
		_common_definitions->setDefinition("SHADER_FRAGMENT_STORES_AND_ATOMICS", tod(features.features2.features.fragmentStoresAndAtomics));

		_common_definitions->setDefinition("SHADER_BUFFER_INT64_ATOMICS", tod(features.features_12.shaderBufferInt64Atomics));
		_common_definitions->setDefinition("SHADER_SHARED_INT64_ATOMICS", tod(features.features_12.shaderSharedInt64Atomics));
		_common_definitions->setDefinition("SHADER_IMAGE_INT64_ATOMICS", tod(features.shader_image_atomic_int64_ext.shaderImageInt64Atomics));
		_common_definitions->setDefinition("SHADER_SPARSE_IMAGE_INT64_ATOMICS", tod(features.shader_image_atomic_int64_ext.sparseImageInt64Atomics));

		uint32_t atomic_float_16_flags = 0;
		uint32_t atomic_float_32_flags = 0;
		uint32_t atomic_float_64_flags = 0;
		if(features.shader_atomic_float_ext.shaderBufferFloat32Atomics)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, BUFFER);
		if(features.shader_atomic_float_ext.shaderBufferFloat32AtomicAdd)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, BUFFER);
		if(features.shader_atomic_float_ext.shaderBufferFloat64Atomics)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, BUFFER);
		if(features.shader_atomic_float_ext.shaderBufferFloat64AtomicAdd)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, BUFFER);
		if(features.shader_atomic_float_ext.shaderSharedFloat32Atomics)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, SHARED_MEMORY);
		if(features.shader_atomic_float_ext.shaderSharedFloat32AtomicAdd)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, SHARED_MEMORY);
		if(features.shader_atomic_float_ext.shaderSharedFloat64Atomics)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, SHARED_MEMORY);
		if(features.shader_atomic_float_ext.shaderSharedFloat64AtomicAdd)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, SHARED_MEMORY);
		if(features.shader_atomic_float_ext.shaderImageFloat32Atomics)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, IMAGE);
		if(features.shader_atomic_float_ext.shaderImageFloat32AtomicAdd)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, IMAGE);
		if(features.shader_atomic_float_ext.sparseImageFloat32Atomics)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, SPARSE_IMAGE);
		if(features.shader_atomic_float_ext.sparseImageFloat32AtomicAdd)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, SPARSE_IMAGE);
		
		if(features.shader_atomic_float_2_ext.shaderBufferFloat16Atomics)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, BUFFER);
		if(features.shader_atomic_float_2_ext.shaderBufferFloat16AtomicAdd)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, BUFFER);
		if(features.shader_atomic_float_2_ext.shaderBufferFloat16AtomicMinMax)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, BUFFER);
		if(features.shader_atomic_float_2_ext.shaderBufferFloat32AtomicMinMax)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, BUFFER);
		if(features.shader_atomic_float_2_ext.shaderBufferFloat64AtomicMinMax)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, BUFFER);
		if(features.shader_atomic_float_2_ext.shaderSharedFloat16Atomics)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, SHARED_MEMORY);
		if(features.shader_atomic_float_2_ext.shaderSharedFloat16AtomicAdd)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, SHARED_MEMORY);
		if(features.shader_atomic_float_2_ext.shaderSharedFloat16AtomicMinMax)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, SHARED_MEMORY);
		if(features.shader_atomic_float_2_ext.shaderSharedFloat32AtomicMinMax)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, SHARED_MEMORY);
		if(features.shader_atomic_float_2_ext.shaderSharedFloat64AtomicMinMax)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, SHARED_MEMORY);
		if(features.shader_atomic_float_2_ext.shaderImageFloat32AtomicMinMax)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, IMAGE);
		if(features.shader_atomic_float_2_ext.sparseImageFloat32AtomicMinMax)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, SPARSE_IMAGE);
		
		_common_definitions->setDefinition("SHADER_ATOMIC_FLOAT_16_FLAGS", std::to_string(atomic_float_16_flags));
		_common_definitions->setDefinition("SHADER_ATOMIC_FLOAT_32_FLAGS", std::to_string(atomic_float_32_flags));
		_common_definitions->setDefinition("SHADER_ATOMIC_FLOAT_64_FLAGS", std::to_string(atomic_float_64_flags));
		
	}


	std::shared_ptr<UpdateContext> ResourcesManager::beginUpdateCycle()
	{
		++_update_tick;

		{
			std::chrono::time_point<_shader_clock_t> now = _shader_clock_t::now();
			if ((now - _last_shader_check) > _shader_check_period)
			{
				++_shader_check_tick;
				_last_shader_check = now;
			}
		}

		_common_definitions->update();

		std::shared_ptr<UpdateContext> res = std::make_shared<UpdateContext>(UpdateContext::CI{
			.app = application(),
			.name = name() + ".update_context",
			.update_tick = _update_tick,
			.shader_check_tick = _shader_check_tick,
			.common_definitions = _common_definitions.get(),
			.mounting_points = _mounting_points.get(),
			.upload_queue = &_upload_queue,
			.mips_queue = &_mips_queue,
			.descriptor_writer = _descriptor_writer,
		});
		return res;
	}


	void ResourcesManager::finishUpdateCycle(std::shared_ptr<UpdateContext> context)
	{
		assert(context);
		ResourcesLists & resources_to_update = context->resourcesToUpdateLater();
		resources_to_update.update(*context);

		_descriptor_writer.record();
	}
}