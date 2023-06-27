#pragma once

#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/ImageView.hpp>

namespace vkl
{
	struct Resource
	{
		std::shared_ptr<Buffer> _buffer = {};
		DynamicValue<Buffer::Range> _buffer_range = {};
		std::shared_ptr<ImageView> _image = {};
		ResourceState2 _begin_state = {};
		std::optional<ResourceState2> _end_state = {}; // None means the same as begin state
		VkImageUsageFlags _image_usage = 0;
		VkBufferUsageFlags _buffer_usage = 0;

		// Can't have a constructor and still an aggregate initialization :'(
		//Resource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image);

		bool isImage() const
		{
			return !!_image;
		}

		bool isBuffer() const
		{
			return !!_buffer;
		}

		const std::string& name()const
		{
			if (isImage())
				return _image->name();
			else// if (isBuffer())
				return _buffer->name();
		}
	};

	Resource MakeResource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image);
}