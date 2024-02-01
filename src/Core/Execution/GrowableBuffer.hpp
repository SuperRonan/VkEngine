#pragma once

#include <Core/VkObjects/Buffer.hpp>

#include <Core/Execution/Executor.hpp>

#include <Core/Commands/TransferCommand.hpp>

namespace vkl
{	
	class GrowableBuffer : public VkObject
	{
	protected:
		
		Dyn<size_t> _size;
		size_t _capacity = 0;

		std::shared_ptr<Buffer> _buffer;
		std::shared_ptr<BufferInstance> _prev_buffer_inst;

		std::shared_ptr<BufferInstance> updateBuffer(UpdateContext& ctx, bool shrink_to_fit);

	public:

		GrowableBuffer(Buffer::CI const& ci);

		virtual void updateResources(UpdateContext & ctx, bool shrink_to_fit = false);

		virtual void recordTransferIFN(ExecutionRecorder & exec);

		CopyBuffer::CopyInfoInstance consumeSynchCopyInfo();

		bool needsTransfer() const
		{
			return _prev_buffer_inst.operator bool();
		}


		std::shared_ptr<Buffer> const& buffer() const
		{
			return _buffer;
		}

		size_t capacity()const
		{
			return _capacity;
		}
	};
}