#include <vkl/Execution/ResourceState.hpp>

namespace vkl
{
	//ResourceStateTracker::BufferStates::BufferStates(BufferInstance* buffer) :
	//	_buffer(buffer)
	//{
	//	Range range = {
	//		.begin = 0,
	//		.len = buffer->createInfo().size,
	//	};
	//	_states[range] = ResourceState2{
	//		.access = VK_ACCESS_2_NONE,
	//		.stage = VK_PIPELINE_STAGE_2_NONE,
	//	};
	//}

	//ResourceState2 ResourceStateTracker::BufferStates::getState(Range const& r)
	//{
	//	// TODO
	//	if (!_states.contains(r))
	//	{
	//		_states[r] = ResourceState2{
	//			.access = VK_ACCESS_2_NONE,
	//			.stage = VK_PIPELINE_STAGE_2_NONE,
	//		};
	//	}
	//	return _states.at(r);
	//}

	//void ResourceStateTracker::BufferStates::setState(Range const& r, ResourceState2 const& state)
	//{
	//	_states[r] = state;
	//}

	//ResourceStateTracker::ImageStates::ImageStates(ImageInstance* image) :
	//	_image(image)
	//{

	//	Range range = {
	//		.aspectMask = getImageAspectFromFormat(_image->createInfo().format),
	//		.baseMipLevel = 0,
	//		.levelCount = _image->createInfo().mipLevels,
	//		.baseArrayLayer = 0,
	//		.layerCount = _image->createInfo().arrayLayers,
	//	};
	//	_states[range] = ResourceState2{
	//		.access = VK_ACCESS_2_NONE,
	//		.layout = _image->createInfo().initialLayout,
	//		.stage = VK_PIPELINE_STAGE_2_NONE,
	//	};
	//}

	//ResourceState2 ResourceStateTracker::ImageStates::getState(Range const& r)
	//{
	//	// TODO
	//	if (!_states.contains(r))
	//	{
	//		_states[r] = ResourceState2{
	//			.access = VK_ACCESS_2_NONE,
	//			.layout = _image->createInfo().initialLayout,
	//			.stage = VK_PIPELINE_STAGE_2_NONE,
	//		};
	//	}
	//	return _states.at(r);
	//}

	//void ResourceStateTracker::ImageStates::setState(Range const& r, ResourceState2 const& state)
	//{
	//	_states[r] = state;
	//}

	//ResourceStateTracker::ResourceStateTracker()
	//{

	//}

	//void ResourceStateTracker::registerImage(ImageInstance* image)
	//{
	//	_images.emplace(image->uniqueId(), image);
	//}

	//void ResourceStateTracker::registerBuffer(BufferInstance* buffer)
	//{
	//	_buffers.emplace(buffer->uniqueId(), buffer);
	//	//_buffers[buffer->uniqueId()] = BufferStates(buffer);
	//}

	//void ResourceStateTracker::releaseImage(ImageInstance* image)
	//{
	//	_images.erase(image->uniqueId());
	//}

	//void ResourceStateTracker::releaseBuffer(BufferInstance* buffer)
	//{
	//	_buffers.erase(buffer->uniqueId());
	//}

	//ResourceState2 ResourceStateTracker::getImageState(ImageKey const& key)
	//{
	//	assert(_images.contains(key.id));
	//	return _images.at(key.id).getState(key.range);
	//}

	//ResourceState2 ResourceStateTracker::getImageState(ImageViewInstance* view)
	//{
	//	return getImageState(view->getResourceKey());
	//}

	//ResourceState2 ResourceStateTracker::getBufferState(BufferKey const& key)
	//{
	//	assert(_buffers.contains(key.id));
	//	return _buffers.at(key.id).getState(key.range);
	//}

	//void ResourceStateTracker::setImageState(ImageKey const& key, ResourceState2 const& state)
	//{
	//	_images[key.id].setState(key.range, state);
	//}

	//void ResourceStateTracker::setImageState(ImageViewInstance* view, ResourceState2 const& state)
	//{
	//	setImageState(view->getResourceKey(), state);
	//}

	//void ResourceStateTracker::setBufferState(BufferKey const& key, ResourceState2 const& state)
	//{
	//	_buffers[key.id].setState(key.range, state);
	//}
}