#include <vkl/Execution/ResourcesManager.hpp>



namespace vkl
{

	ResourcesManager::ResourcesManager(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_shader_check_period(ci.shader_check_period),
		_common_definitions(std::make_unique<DefinitionsMap>()),
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