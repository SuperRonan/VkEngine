#include "ExecutionContext.hpp"
#include <cassert>

namespace vkl
{
	//Resource::Resource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> view):
	//	_buffers({buffer}),
	//	_images({view})
	//{}
	
	ExecutionContext::ExecutionContext(CreateInfo const& ci) :
		_command_buffer(ci.cmd),
		_reosurce_states(ci.rst),
		_staging_pool(ci.staging_pool)
	{}

    ResourceState2& ExecutionContext::getBufferState(std::shared_ptr<Buffer> b)
	{
		const VkBuffer key = *b->instance();
		if (!_reosurce_states->_buffer_states.contains(key))
		{
			ResourceState2 not_yet_used{};
			_reosurce_states->_buffer_states[key] = not_yet_used;
		}
		return _reosurce_states->_buffer_states[key];
	}

	ResourceState2& ExecutionContext::getImageState(std::shared_ptr<ImageView> i)
	{
		const ImageRange ir = {
			.image = *i->image()->instance(),
			.range = i->range(),
		};
		if (!_reosurce_states->_image_states.contains(ir))
		{
			ResourceState2 not_yet_used{};
			_reosurce_states->_image_states[ir] = not_yet_used;
		}
		return _reosurce_states->_image_states[ir];
	}

	void ExecutionContext::setBufferState(std::shared_ptr<Buffer> b, ResourceState2 const& s)
	{
		_reosurce_states->_buffer_states[*b->instance()] = s;
	}

	void ExecutionContext::setImageState(std::shared_ptr<ImageView> v, ResourceState2 const& s)
	{
		const ImageRange ir = {
			.image = *v->image()->instance(),
			.range = v->range(),
		};
		_reosurce_states->_image_states[ir] = s;
	}

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
