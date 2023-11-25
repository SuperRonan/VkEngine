#include "UploadQueue.hpp"

namespace vkl
{
	size_t AsynchUpload::getSize() const
	{
		return source.size();
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
					.sources = {
						PositionedObjectView{
							.obj = std::move(aquired.source),
							.pos = aquired.target_buffer_offset,
						},
					},
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
					.buffer_row_length = 0,
					.buffer_image_height = 0,
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