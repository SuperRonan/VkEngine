#include <vkl/Execution/ResourcesManager.hpp>

#include <vkl/IO/DependencyTracker.hpp>

namespace vkl
{

	ResourcesManager::ResourcesManager(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_auto_file_check_period(ci.auto_file_check_period),
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
		_dependencies_tracker = std::make_unique<DependencyTracker>(DependencyTracker::CI{
			.file_system = application()->fileSystem(),
			.executor = &application()->threadPool(),
			.log = &application()->logger(),
			.period = _auto_file_check_period,
		});
		populateCommonObjects();
	}

	ResourcesManager::~ResourcesManager()
	{

	}

	void ResourcesManager::populateCommonObjects()
	{
		
	}


	std::shared_ptr<UpdateContext> ResourcesManager::beginUpdateCycle()
	{
		++_update_tick;

		{
			application()->fileSystem()->resetCache();
			_dependencies_tracker->update();
		}

		_common_definitions->update();

		std::shared_ptr<UpdateContext> res = std::make_shared<UpdateContext>(UpdateContext::CI{
			.app = application(),
			.name = name() + ".update_context",
			.update_tick = _update_tick,
			.dependencies_tracker = _dependencies_tracker.get(),
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