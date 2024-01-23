#pragma once

#include <Core/VkObjects/Buffer.hpp>

#include <Core/Execution/Executor.hpp>

namespace vkl
{
	class GrowableBuffer : public VkObject
	{
	protected:
		
		Dyn<size_t> _size;
		size_t _capacity = 0;

		std::shared_ptr<Buffer> _buffer;
		std::shared_ptr<BufferInstance> _prev_buffer_inst;

	public:

		GrowableBuffer(Buffer::CI const& ci);

		void updateResources(UpdateContext & ctx, bool shrink_to_fit = false);

		void recordTransferIFN(ExecutionRecorder & exec);

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