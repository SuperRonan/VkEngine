#include "ExecutionContext.hpp"
#include <cassert>

namespace vkl
{
	//Resource::Resource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> view):
	//	_buffers({buffer}),
	//	_images({view})
	//{}
	
	ExecutionContext::ExecutionContext(ResourceStateTracker * rst, std::shared_ptr<CommandBuffer> cmd):
		_command_buffer(cmd),
		_reosurce_states(rst)
	{}

    ResourceState& ExecutionContext::getBufferState(VkBuffer b)
	{
		if (!_reosurce_states->_buffer_states.contains(b))
		{
			ResourceState not_yet_used{};
			_reosurce_states->_buffer_states[b] = not_yet_used;
		}
		return _reosurce_states->_buffer_states[b];
	}

	ResourceState& ExecutionContext::getImageState(VkImageView i)
	{
		if (!_reosurce_states->_image_states.contains(i))
		{
			ResourceState not_yet_used{};
			_reosurce_states->_image_states[i] = not_yet_used;
		}
		return _reosurce_states->_image_states[i];
	}

	void ExecutionContext::setBufferState(VkBuffer b, ResourceState const& s)
	{
		_reosurce_states->_buffer_states[b] = s;
	}

	void ExecutionContext::setImageState(VkImageView v, ResourceState const& s)
	{
		_reosurce_states->_image_states[v] = s;
	}

	void ExecutionContext::setCommandBuffer(std::shared_ptr<CommandBuffer> cmd)
	{
		_command_buffer = cmd;
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
		Resource res;
		if (!!buffer)
			res._buffers.push_back(buffer);
		else if (!!image)
			res._images.push_back(image);
		return res;

	}
}