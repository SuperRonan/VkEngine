#include "ResourcesManager.hpp"

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

		if (features.features_12.shaderFloat16)
		{
			_common_definitions->setDefinition("SHADER_FP16_AVAILABLE", "1");
		}

		if (features.ray_query_khr.rayQuery)
		{
			_common_definitions->setDefinition("SHADER_RAY_QUERY_AVAILABLE", "1");
		}
		if (features.ray_tracing_pipeline_khr.rayTracingPipeline)
		{
			_common_definitions->setDefinition("SHADER_RAY_TRACING_AVAILABLE", "1");
		}
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