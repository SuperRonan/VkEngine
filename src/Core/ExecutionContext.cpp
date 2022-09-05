#include "ExecutionContext.hpp"
#include <cassert>

namespace vkl
{
    ResourceState& ExecutionContext::getBufferState(VkBuffer b)
	{
		if (!_buffer_states.contains(b))
		{
			ResourceState not_yet_used{};
			_buffer_states[b] = not_yet_used;
		}
		return _buffer_states[b];
	}

	ResourceState& ExecutionContext::getImageState(VkImageView i)
	{
		if (!_image_states.contains(i))
		{
			ResourceState not_yet_used{};
			_image_states[i] = not_yet_used;
		}
		return _image_states[i];
	}

	const std::string& Resource::name()const
	{
		// front?
		if (isImage())
			return _images.front()->name();
		else if (isBuffer())
			return _buffers.front()->name();
		else
			return {};
	}
}