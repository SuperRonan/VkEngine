#include <vkl/Commands/ResourceUsageList.hpp>

namespace vkl
{
	void FullyMergedBufferUsageList::add(BufferAndRangeInstance const& bari, ResourceState2 const& state, std::optional<ResourceState2> const& end_state, VkBufferUsageFlags2KHR usages)
	{
		assert(bari);
		BufferSubRangeState& bsrs = _buffers[bari.buffer];
		Buffer::Range range = bari.range;
		if (range.len == 0)
		{
			range.len = (bari.buffer->createInfo().size - range.begin);
		}
		if (bsrs.range.len == 0)
		{
			bsrs.range = range;
			bsrs.state = state;
			bsrs.end_state = end_state;
		}
		else
		{
			size_t new_end = std::max(range.end(), bsrs.range.end());
			size_t new_begin = std::min(range.begin, bsrs.range.begin);
			bsrs.range.begin = new_begin;
			bsrs.range.len = new_end - new_begin;

			bsrs.state |= state;

			if (!bsrs.end_state.has_value() && end_state.has_value())
			{
				bsrs.end_state = end_state.value();
			}
			else if (bsrs.end_state.has_value() && end_state.has_value())
			{
				assertm(false, "Cannot set buffer end state multiple times!");
			}
		}
		bsrs.usage |= usages;
	}

	void FullyMergedBufferUsageList::add(FullyMergedBufferUsageList const& other)
	{
		for (const auto& [buffer, state] : other._buffers)
		{
			BufferSegmentInstance segment{
				.buffer = buffer,
				.range = state.range,
			};
			add(segment, state.state, state.end_state, state.usage);
		}
	}

	void FullyMergedBufferUsageList::iterate(BufferUsageFunction const& fn) const
	{
		for (const auto& [bi, u] : _buffers)
		{
			assert(bi);
			fn(bi, &u, 1);
		}
	}

	void FullyMergedBufferUsageList::clear()
	{
		_buffers.clear();
	}





	void FullyMergedImageUsageList::add(std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state, VkImageUsageFlags usages)
	{
		ImageSubRangeState& isrs = _images[ii];
		if (isrs.range.aspectMask == VK_IMAGE_ASPECT_NONE)
		{
			isrs.range = range;
			isrs.state = state;
			isrs.end_state = end_state;
		}
		else
		{
			const uint32_t mip_end = std::max(isrs.range.baseMipLevel + isrs.range.levelCount, range.baseMipLevel + range.levelCount);
			const uint32_t layer_end = std::max(isrs.range.baseArrayLayer + isrs.range.layerCount, range.baseArrayLayer + range.layerCount);
			VkImageSubresourceRange new_range{
				.aspectMask = isrs.range.aspectMask | range.aspectMask,
				.baseMipLevel = std::min(isrs.range.baseMipLevel, range.baseMipLevel),
				.baseArrayLayer = std::min(isrs.range.baseArrayLayer, range.baseArrayLayer),
			};
			new_range.levelCount = mip_end - new_range.baseMipLevel;
			new_range.layerCount = layer_end - new_range.baseArrayLayer;

			isrs.range = new_range;
			isrs.state |= state;

			if (isrs.state.layout != state.layout)
			{
				assertm(false, "Cannot use the same image with different layouts");
			}

			if (!isrs.end_state.has_value() && end_state.has_value())
			{
				isrs.end_state = end_state.value();
			}
			else if (isrs.end_state.has_value() && end_state.has_value())
			{
				assertm(false, "Cannot set image end state multiple times!");
			}
		}
		isrs.usage |= usages;
	}

	void FullyMergedImageUsageList::add(FullyMergedImageUsageList const& other)
	{
		for (const auto& [img, state] : other._images)
		{
			add(img, state.range, state.state, state.end_state, state.usage);
		}
	}

	void FullyMergedImageUsageList::iterate(ImageUsageFunction const& fn) const
	{
		for (const auto& [ii, u] : _images)
		{
			fn(ii, &u, 1);
		}
	}

	void FullyMergedImageUsageList::clear()
	{
		_images.clear();
	}

	



	ModularResourceUsageList::ModularResourceUsageList()
	{

	}

	void ModularResourceUsageList::addBuffer(BufferAndRangeInstance const& bari, ResourceState2 const& state, std::optional<ResourceState2> const& end_state, VkBufferUsageFlags2KHR usages)
	{
		_buffers.add(bari, state, end_state, usages);
	}

	void ModularResourceUsageList::addImage(std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state, VkImageUsageFlags usages)
	{
		_images.add(ii, range, state, end_state, usages);
	}

	void ModularResourceUsageList::iterateOnBuffers(BufferUsageFunction const& fn) const
	{
		_buffers.iterate(fn);
	}

	void ModularResourceUsageList::iterateOnImages(ImageUsageFunction const& fn) const
	{
		return _images.iterate(fn);
	}

	void ModularResourceUsageList::clear()
	{
		AbstractResourceUsageList::clear();
		_buffers.clear();
		_images.clear();
	}
}