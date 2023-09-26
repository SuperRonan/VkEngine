#include "ExecutionContext.hpp"
#include <cassert>

namespace vkl
{
	ExecutionContext::ExecutionContext(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_command_buffer(ci.cmd),
		_resource_tid(ci.resource_tid),
		_staging_pool(ci.staging_pool),
		_mounting_points(ci.mounting_points),
		_graphics_bound_sets(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + "._gfx_bound_sets",
			.cmd = _command_buffer,
			.pipeline_binding = VK_PIPELINE_BIND_POINT_GRAPHICS,
		}),
		_compute_bound_sets(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + "._cmp_bound_sets",
			.cmd = _command_buffer,
			.pipeline_binding = VK_PIPELINE_BIND_POINT_COMPUTE,
		}),
		_ray_tracing_bound_sets(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + "._rt_bound_sets",
			.cmd = _command_buffer,
			.pipeline_binding = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		})
	{}
}
