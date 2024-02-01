#pragma once

#include "GrowableBuffer.hpp"
#include <Core/Maths/AlignedAxisBoundingBox.hpp>


namespace vkl
{
	class HostManagedBuffer : public GrowableBuffer
	{
	protected:

		size_t _byte_size = 0;
		void * _data = nullptr;

		AABB<1, size_t> _upload_range;

		PositionedObjectView consumeUploadView();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			size_t size = 0;
			VkBufferUsageFlags usage = 0;
			VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;
		};
		using CI = CreateInfo;

		HostManagedBuffer(CreateInfo const& ci);

		virtual ~HostManagedBuffer() override;

		size_t byteSize()const
		{
			return _byte_size;
		}

		void grow(size_t desired_size);

		void resize(size_t byte_size);

		void set(size_t offset, const void * data, size_t len);

		template <class T>
		void set(size_t index, T const& t)
		{
			const size_t byte_address = sizeof(T) * index;
			const Buffer::Range t_range{
				.begin = byte_address, 
				.len = sizeof(T),
			};

			invalidateByteRange(t_range);

			if (t_range.end() > _byte_size)
			{
				grow(t_range.end());
			}
			assert(sizeof(T) * (index + 1) <= _byte_size);
			T * t_data = (T*)_data;
			t_data[index] = t;
		}

		template <class T>
		const T& get(size_t index)const
		{
			assert(sizeof(T) * (index + 1) <= _byte_size);
			T* t_data = (T*)_data;
			return t_data[index];
		}


		void invalidateByteRange(Buffer::Range const& range);

		template <class T>
		void invalidateRange(Range_st range)
		{
			const Buffer::Range byte_range = {
				.begin = range.begin * sizeof(T),
				.len = range.len * sizeof(T),
			};
			invalidateByteRange(byte_range);
		}


		virtual void updateResources(UpdateContext & ctx, bool shrink_to_fit = false) override;

		virtual void recordTransferIFN(ExecutionRecorder & exec) override;
	};
}