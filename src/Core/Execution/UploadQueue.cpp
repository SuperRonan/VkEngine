#include "UploadQueue.hpp"

#include <Core/VkObjects/ImageView.hpp>

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

	ResourcesToUpload UploadQueue::consume(Budget const& b)
	{
		Budget total;
		ResourcesToUpload res;

		_mutex.lock();

		while (!_queue.empty())
		{
			AsynchUpload aquired = std::move(_queue.front());
			_queue.pop_front();

			total += aquired.getSize();

			if (aquired.target_buffer)
			{
				static thread_local MyVector<ResourcesToUpload::BufferSource> sources;
				sources.resize(aquired.sources.size());
				for (size_t i = 0; i < sources.size(); ++i)
				{
					sources[i] = ResourcesToUpload::BufferSource{
						.data = aquired.sources[i].obj.data(),
						.size = aquired.sources[i].obj.size(),
						.offset = aquired.sources[i].pos,
						.copy_data = aquired.sources[i].obj.ownsValue(),
					};
				}
				ResourcesToUpload::BufferUpload bu{
					.sources = sources.data(),
					.sources_count = sources.size(),
					.dst = std::move(aquired.target_buffer),
					.completion_callback = std::move(aquired.completion_callback),
				};
				res += std::move(bu);
			}
			else
			{
				assert(!!aquired.target_view);
				ResourcesToUpload::ImageUpload iu{
					.data = aquired.source.data(),
					.size = aquired.source.size(),
					.copy_data = aquired.source.ownsValue(),
					.buffer_row_length = aquired.buffer_row_length,
					.buffer_image_height = aquired.buffer_image_height,
					.dst = std::move(aquired.target_view),
					.completion_callback = std::move(aquired.completion_callback),
				};
				res += std::move(iu);
			}

			if (!total.withinLimit(b))
			{
				break;
			}
		}

		_mutex.unlock();

		return res;
	}



	size_t AsynchMipsCompute::getSize()const
	{
		size_t res = 0;

		const VkExtent3D & ext = target->image()->createInfo().extent;
		const size_t l = target->createInfo().subresourceRange.layerCount;

		res = ext.width * ext.height * ext.depth * l;
		// TODO include the format size
		return res;
	}


	MipMapComputeQueue::MipMapComputeQueue(CreateInfo const& ci):
		VkObject(ci.app, ci.name)
	{}

	void MipMapComputeQueue::enqueue(AsynchMipsCompute const& mc)
	{
		std::unique_lock lock(_mutex);
		_queue.push_back(mc);
	}

	void MipMapComputeQueue::enqueue(AsynchMipsCompute && mc)
	{
		std::unique_lock lock(_mutex);
		_queue.push_back(std::move(mc));
	}

	MyVector<AsynchMipsCompute> MipMapComputeQueue::consume(Budget const& b)
	{
		MyVector<AsynchMipsCompute> res;
		std::unique_lock lock(_mutex);

		Budget total;

		while (!_queue.empty())
		{
			AsynchMipsCompute aquired = std::move(_queue.front());
			_queue.pop_front();

			total += aquired.getSize();

			res.push_back(std::move(aquired));

			if (!total.withinLimit(b))
			{
				break;
			}
		}
		return res;
	}
}