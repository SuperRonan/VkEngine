#include "GrowableBuffer.hpp"

namespace vkl
{
	GrowableBuffer::GrowableBuffer(Buffer::CI const& ci):
		VkObject(ci.app, ci.name),
		_size(ci.size)
	{
		_capacity = _size.value();

		Buffer::CI _ci = ci;

		_ci.size = &_capacity;

		_buffer = std::make_shared<Buffer>(_ci);
	}

	std::shared_ptr<BufferInstance> GrowableBuffer::updateBuffer(UpdateContext& ctx, bool shrink_to_fit)
	{
		const size_t size = _size.value();

		if (size > _capacity)
		{
			_capacity = std::max(size, _capacity * 2);
		}

		if (shrink_to_fit)
		{
			_capacity = size;
		}

		std::shared_ptr<BufferInstance> bi = _buffer->instance();
		_buffer->updateResource(ctx);
		return bi;
	}

	void GrowableBuffer::updateResources(UpdateContext & ctx, bool shrink_to_fit)
	{
		std::shared_ptr<BufferInstance> bi = updateBuffer(ctx, shrink_to_fit);
		if (bi != _buffer->instance() && !_prev_buffer_inst)
		{
			_prev_buffer_inst = bi;
		}
	}

	void GrowableBuffer::recordTransferIFN(ExecutionRecorder& exec)
	{
		if (needsTransfer())
		{
			CopyBuffer cp = CopyBuffer::CI{
				.app = application(),
			};

			exec(cp.with(consumeSynchCopyInfo()));
		}
	}

	CopyBuffer::CopyInfoInstance GrowableBuffer::consumeSynchCopyInfo()
	{
		assert(needsTransfer());

		CopyBuffer::CopyInfoInstance res{
			.src = _prev_buffer_inst,
			.dst = _buffer->instance()
		};

		_prev_buffer_inst.reset();

		return res;
	}
}