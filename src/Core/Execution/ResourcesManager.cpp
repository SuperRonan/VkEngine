#include "ResourcesManager.hpp"

namespace vkl
{

	ResourcesManager::ResourcesManager(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_shader_check_period(ci.shader_check_period),
		_common_definitions(ci.common_definitions),
		_mounting_points(ci.mounting_points),
		_upload_queue(ci.upload_queue)
	{
		_last_shader_check = _shader_clock_t::now() - 2 * _shader_check_period;
	}


	std::shared_ptr<UpdateContext> ResourcesManager::beginUpdateCycle()
	{
		++_update_cycle;

		{
			std::chrono::time_point<_shader_clock_t> now = _shader_clock_t::now();
			if ((now - _last_shader_check) > _shader_check_period)
			{
				++_shader_check_cycle;
				_last_shader_check = now;
			}
		}

		std::shared_ptr<UpdateContext> res = std::make_shared<UpdateContext>(UpdateContext::CI{
			.app = application(),
			.name = name() + ".update_context",
			.update_cycle = _update_cycle,
			.shader_check_cycle = _shader_check_cycle,
			.common_definitions = _common_definitions,
			.mounting_points = _mounting_points,
			.upload_queue = _upload_queue,
		});
		return res;
	}


	void ResourcesManager::finishUpdateCycle(std::shared_ptr<UpdateContext> context)
	{
		assert(context);
		ResourcesLists & resources_to_update = context->resourcesToUpdateLater();
		resources_to_update.update(*context);
	}
}