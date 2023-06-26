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

	const std::string& Resource::name()const
	{
		// front?
		if (isImage())
			return _image->name();
		else// if (isBuffer())
			return _buffer->name();
	}

	Resource MakeResource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image)
	{
		Resource res;
		if (!!buffer)
			res._buffer = buffer;
		else if (!!image)
			res._image = image;
		return res;

	}
}
