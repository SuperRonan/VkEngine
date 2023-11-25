#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/Execution/ResourcesToUpload.hpp>
#include <mutex>

namespace vkl
{

	struct AsynchUpload
	{
		std::string name = {};
		std::vector<PositionedObjectView> sources = {};
		ObjectView source = {};
		
		std::shared_ptr<ImageView> target_view = nullptr;
		uint32_t buffer_row_length = 0;
		uint32_t buffer_image_height = 0;
		
		std::shared_ptr<Buffer> target_buffer = nullptr;

		CompletionCallback completion_callback = {};

		size_t getSize()const;
	};

	class UploadQueue : public VkObject
	{
	public:

	protected:

		std::mutex _mutex;
		std::deque<AsynchUpload> _queue;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		UploadQueue(CreateInfo const& ci);

		void enqueue(AsynchUpload const& upload);
		void enqueue(AsynchUpload && upload);

		ResourcesToUpload consume(size_t budget);
	};
}