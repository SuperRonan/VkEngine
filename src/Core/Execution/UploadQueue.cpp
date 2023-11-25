#include "UploadQueue.hpp"

namespace vkl
{
	size_t AsynchUpload::getSize() const
	{
		size_t res = 0;
		for (const auto& src : sources)
		{
			res += src.obj.size();
		}
		return res;
	}

	UploadQueue::UploadQueue(CreateInfo const& ci) :
		VkObject(ci.app, ci.name)
	{

	}

	void UploadQueue::enqueue(AsynchUpload const& upload)
	{
		_mutex.lock();
		{
			_queue.push_back(upload);
		}
		_mutex.unlock();
	}

	void UploadQueue::enqueue(AsynchUpload && upload)
	{
		_mutex.lock();
		{
			_queue.emplace_back(std::move(upload));
		}
		_mutex.unlock();
	}

	ResourcesToUpload UploadQueue::consume(size_t budget)
	{
		size_t total = 0;
		ResourcesToUpload res;

		_mutex.lock();

		while (!_queue.empty())
		{
			AsynchUpload aquired = std::move(_queue.front());
			_queue.pop_front();

			total += aquired.getSize();

			if (aquired.target_buffer)
			{
				ResourcesToUpload::BufferUpload bu{
					.sources = std::move(aquired.sources),
					.dst = std::move(aquired.target_buffer),
					.completion_callback = std::move(aquired.completion_callback),
				};
				res += std::move(bu);
			}
			else
			{
				assert(!!aquired.target_view);
				ResourcesToUpload::ImageUpload iu{
					.src = std::move(aquired.source),
					.buffer_row_length = aquired.buffer_row_length,
					.buffer_image_height = aquired.buffer_image_height,
					.dst = std::move(aquired.target_view),
					.completion_callback = std::move(aquired.completion_callback),
				};
				res += std::move(iu);
			}

			if (total >= budget)
			{
				break;
			}
		}

		_mutex.unlock();

		return res;
	}
}