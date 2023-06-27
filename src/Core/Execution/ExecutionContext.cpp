#include "ExecutionContext.hpp"
#include <cassert>

namespace vkl
{
	ExecutionContext::ExecutionContext(CreateInfo const& ci) :
		_command_buffer(ci.cmd),
		_resource_tid(ci.resource_tid),
		_staging_pool(ci.staging_pool)
	{}

	void ExecutionContext::setCommandBuffer(std::shared_ptr<CommandBuffer> cmd)
	{
		_command_buffer = cmd;
	}
}
