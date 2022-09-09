#include "ExecutionContext.hpp"
#include <cassert>

namespace vkl
{
	//Resource::Resource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> view):
	//	_buffers({buffer}),
	//	_images({view})
	//{}

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

	void ExecutionContext::setBufferState(VkBuffer b, ResourceState const& s)
	{
		_buffer_states[b] = s;
	}

	void ExecutionContext::setImageState(VkImageView v, ResourceState const& s)
	{
		_image_states[v] = s;
	}

	const std::string& Resource::name()const
	{
		// front?
		if (isImage())
			return _images.front()->name();
		else// if (isBuffer())
			return _buffers.front()->name();
	}

	Resource MakeResource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image)
	{
		return Resource{
			._buffers = {buffer},
			._images = {image},
		};
	}
}