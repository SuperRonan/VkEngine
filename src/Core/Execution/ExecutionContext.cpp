#include "ExecutionContext.hpp"
#include <cassert>

namespace vkl
{
	ExecutionContext::ExecutionContext(CreateInfo const& ci) :
		_command_buffer(ci.cmd),
		_resource_tid(ci.resource_tid),
		_staging_pool(ci.staging_pool),
		_mounting_points(ci.mounting_points)
	{}
}
