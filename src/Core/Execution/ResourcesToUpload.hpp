#pragma once

#include <Core/VulkanCommons.hpp>
#include <Core/Execution/CompletionCallback.hpp>
#include <vector>

namespace vkl
{
	
	class ImageViewInstance;
	class BufferInstance;
	
	struct ResourcesToUpload
	{
		// Warning: There is a bug here!
		// The source ObjectView (for both) is not guarantied to be available (if not owned) because the issuer might have destroyed it!
		// We have to guaranty it!
		// Potential solutions: 
		// - A shared ownership of the ObjectView (not a good idea imo)
		// - Do something like AsynchTask:
		// Add a status to the upload object, which can be cancelled
		// The issuer will have to keep a sptr to the upload 
		// Note: Maybe solve this issue in the UploadQueue

		struct ImageUpload
		{
			ObjectView src;
			// 0 -> tightly packed
			uint32_t buffer_row_length = 0;
			uint32_t buffer_image_height = 0;
			std::shared_ptr<ImageViewInstance> dst;
			CompletionCallback completion_callback = {};
		};

		MyVector<ImageUpload> images;

		// Don't like the vector in vector
		struct BufferUpload
		{
			Array<PositionedObjectView> sources;
			std::shared_ptr<BufferInstance> dst;
			CompletionCallback completion_callback = {};
		};

		MyVector<BufferUpload> buffers;

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

		void clear();
	};
}