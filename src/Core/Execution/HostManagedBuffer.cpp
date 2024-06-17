#include "HostManagedBuffer.hpp"

#include <Core/Commands/PrebuiltTransferCommands.hpp>

#include <Core/Execution/Executor.hpp>

namespace vkl
{
	HostManagedBuffer::HostManagedBuffer(CreateInfo const& ci) :
		GrowableBuffer(Buffer::CI{
			.app = ci.app,
			.name = ci.name,
			.size = &_byte_size,
			.min_align = ci.min_align,
			.usage = ci.usage,
			.mem_usage = ci.mem_usage,
		}),
		_byte_size(ci.size),
		_data(_byte_size ? std::malloc(_byte_size) : nullptr)
	{
		_capacity = _byte_size; // hack because _byte_size was not initialized when _capacity was init
	}

	HostManagedBuffer::~HostManagedBuffer()
	{
		if (_data)
		{
			std::free(_data);
			_data = nullptr;
		}

		_byte_size = 0;
		_buffer.reset();
	}

	void HostManagedBuffer::grow(size_t desired_size)
	{
		assert(desired_size > _byte_size);
		const size_t old_size = _byte_size;
		const size_t new_size = std::max(desired_size, _byte_size * 2);
		assert(new_size > old_size);
		void * old_data = _data;


		const bool use_realloc = true;

		if constexpr (use_realloc)
		{
			_data = std::realloc(_data, new_size);
			_byte_size = new_size;
		}
		else
		{
			_data = std::malloc(new_size);
			_byte_size = new_size;
		
			if (_data && old_data)
			{
				std::memcpy(_data, old_data, old_size);
			}

			if (old_data)
			{
				std::free(old_data);
			}
		}
		//g_mutex.lock();
		//std::cout << "Growing " << name() + " from " << old_size << " to " << new_size << std::endl;
		//if (old_data != _data)
		//{
		//	std::cout << "in a new block" << std::endl;
		//}
		//g_mutex.unlock();
		assert(_data);
	}

	void HostManagedBuffer::resize(size_t byte_size)
	{
		grow(byte_size);
	}

	bool HostManagedBuffer::setIFN(size_t offset, const void* data, size_t len)
	{

		bool res = false;
		if (offset + len > _byte_size)
		{
			res = true;
		}
		if (!res)
		{
			res = std::memcmp(data, static_cast<const uint8_t*>(_data) + offset, len) == 0;
		}
		if (res)
		{
			set(offset, data, len);
		}
		return res;
	}

	void HostManagedBuffer::set(size_t offset, const void* data, size_t len)
	{
		const Buffer::Range range{
			.begin = offset,
			.len = len,
		};
		invalidateByteRange(range);
		if (range.end() > _byte_size)
		{
			grow(range.end());
		}
		std::memcpy((std::uint8_t*)_data + offset, data, len);
	}

	void HostManagedBuffer::invalidateByteRange(Buffer::Range const& range)
	{
		const size_t align = 4;
		AABB<1, size_t> segment(Vector<1, size_t>(std::alignDown(range.begin, align)), Vector<1, size_t>(std::alignUp(range.end(), align)));
		_upload_range += segment;
	}

	PositionedObjectView HostManagedBuffer::consumeUploadView()
	{
		assert(!_upload_range.empty());
		Buffer::Range range{
			.begin = _upload_range.bottom().x,
			.len = std::min(_upload_range.diagonal().x, _byte_size),
		};
		_upload_range.reset();
		ObjectView o((const uint8_t*)_data + range.begin, range.len);
		return PositionedObjectView{
			.obj = o,
			.pos = range.begin,
		};
	}

	void HostManagedBuffer::updateResources(UpdateContext& ctx, bool shrink_to_fit)
	{
		GrowableBuffer::updateResources(ctx, shrink_to_fit);
		const bool transfer = _prev_buffer_inst.operator bool();
		if (!transfer)
		{
			if (!_upload_range.empty())
			{
				ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
					.sources = {
						consumeUploadView(),
					},
					.dst = _buffer->instance(),
				};
			}
		}

	}

	void HostManagedBuffer::recordTransferIFN(ExecutionRecorder& exec)
	{
		GrowableBuffer::recordTransferIFN(exec);
		if (!_upload_range.empty())
		{
			UploadBuffer & uploader = application()->getPrebuiltTransferCommands().upload_buffer;
			exec(uploader.with(UploadBuffer::UploadInfo{
				.sources = {
					consumeUploadView(),
				},
				.dst = _buffer,
				.use_update_buffer_ifp = true,
			}));
		}
	}
}