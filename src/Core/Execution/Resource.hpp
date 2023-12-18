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
		Array<BufferAndRangeInstance> buffers = {};
		Array<std::shared_ptr<ImageViewInstance>> images = {};
		ResourceState2 begin_state = {};
		std::optional<ResourceState2> end_state = {}; // None means the same as begin state
		VkFlags usage = 0;

	};

	struct Resource
	{
		Array<BufferAndRange> buffers = {};
		Array<std::shared_ptr<ImageView>> images = {};
		ResourceState2 begin_state = {};
		std::optional<ResourceState2> end_state = {}; // None means the same as begin state
		VkFlags usage = 0;

		// Can't have a constructor and still an aggregate initialization :'(
		//Resource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image);

		std::string_view nameIFP()const
		{
			std::string_view res;
			if (buffers && buffers.front().buffer)
			{
				res = buffers.front().buffer->name();
			}
			else if (images && images.front())
			{
				res = images.front()->name();
			}
			return res;
		}

		ResourceInstance getInstance()
		{
			ResourceInstance res;
			if (buffers)
			{
				res.buffers.resize(buffers.size());
				for (size_t i = 0; i < buffers.size(); ++i)
				{
					if (buffers[i].buffer)
					{
						res.buffers[i] = buffers[i].getInstance();
					}
				}
			}
			else if (images)
			{
				res.images.resize(images.size());
				for (size_t i = 0; i < images.size(); ++i)
				{
					if (images[i])
					{
						res.images[i] = images[i]->instance();
					}
				}
			}

			res.begin_state = begin_state;
			res.end_state = end_state;
			res.usage = usage;
			return res;
		}
	};

	using ResourcesInstances = std::vector<ResourceInstance>;
	using Resources = std::vector<Resource>;
}