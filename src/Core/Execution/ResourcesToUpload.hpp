#pragma once

#include <Core/VulkanCommons.hpp>
#include <Core/Execution/CompletionCallback.hpp>
#include <vector>

namespace vkl
{
	
	class ImageView;
	class Buffer;
	
	struct ResourcesToUpload
	{
		struct ImageUpload
		{
			ObjectView src;
			// 0 -> tightly packed
			uint32_t buffer_row_length = 0;
			uint32_t buffer_image_height = 0;
			std::shared_ptr<ImageView> dst;
			CompletionCallback completion_callback = {};
		};

		std::vector<ImageUpload> images;

		struct BufferUpload
		{
			std::vector<PositionedObjectView> sources;
			std::shared_ptr<Buffer> dst;
			CompletionCallback completion_callback = {};
		};

		std::vector<BufferUpload> buffers;

		ResourcesToUpload& operator+=(ResourcesToUpload const& o);

		ResourcesToUpload& operator+=(ImageUpload const& iu);

		ResourcesToUpload& operator+=(BufferUpload const& bu);

		ResourcesToUpload& operator+=(ResourcesToUpload && o);

		ResourcesToUpload& operator+=(ImageUpload && iu);

		ResourcesToUpload& operator+=(BufferUpload && bu);


		ResourcesToUpload operator+(ResourcesToUpload const& o) const;

		ResourcesToUpload operator+(ImageUpload const& iu) const;

		ResourcesToUpload operator+(BufferUpload const& bu) const;

		size_t getSize()const;
	};
}