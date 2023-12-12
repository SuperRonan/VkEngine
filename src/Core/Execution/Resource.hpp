#pragma once

#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/ImageView.hpp>

namespace vkl
{
	enum class ResourceFlags
	{
		None = 0,
		Buffer = 1,
		Image = 2,
		Array = 4,
		SingleBuffer = Buffer,
		BufferArray = Buffer | Array,
		SingleImage = Image,
		ImageArray = Image | Array,
	};



	struct ResourceInstance
	{
		std::shared_ptr<BufferInstance> buffer = {};
		Buffer::Range buffer_range = {};
		std::shared_ptr<ImageViewInstance> image_view = {};
		ResourceState2 begin_state = {};
		std::optional<ResourceState2> end_state = {}; // None means the same as begin state
		VkImageUsageFlags image_usage = 0;
		VkBufferUsageFlags buffer_usage = 0;


		bool isImage() const
		{
			return !!image_view;
		}

		bool isBuffer() const
		{
			return !!buffer;
		}
	};

	struct Resource
	{
		std::shared_ptr<Buffer> buffer = {};
		DynamicValue<Buffer::Range> buffer_range = {};
		std::shared_ptr<ImageView> image_view = {};
		ResourceState2 begin_state = {};
		std::optional<ResourceState2> end_state = {}; // None means the same as begin state
		VkImageUsageFlags image_usage = 0;
		VkBufferUsageFlags buffer_usage = 0;

		// Can't have a constructor and still an aggregate initialization :'(
		//Resource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image);

		bool isImage() const
		{
			return !!image_view;
		}

		bool isBuffer() const
		{
			return !!buffer;
		}

		const std::string& name()const
		{
			if (isImage())
				return image_view->name();
			else// if (isBuffer())
				return buffer->name();
		}

		ResourceInstance getInstance()
		{
			ResourceInstance res;
			if (buffer)
			{
				res.buffer = buffer->instance();
				res.buffer_range = buffer_range.valueOr(Buffer::Range{});
			}
			else if (image_view)
			{
				res.image_view = image_view->instance();
			}
			res.begin_state = begin_state;
			res.end_state = end_state;
			res.image_usage = image_usage;
			res.buffer_usage = buffer_usage;
			return res;
		}
	};

	using ResourcesInstances = std::vector<ResourceInstance>;
	using Resources = std::vector<Resource>;
}