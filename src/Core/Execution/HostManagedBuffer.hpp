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

		using UploadRange = AABB<1, size_t>;

		union
		{
			UploadRange _upload_range;
			MyVector<UploadRange> _upload_ranges;
		};
		bool _use_single_upload_range = false;

		PositionedObjectView consumeUploadView();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			bool use_single_upload_range = true;
			size_t size = 0;
			VkDeviceSize min_align = 1;
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

		void resizeIFN(size_t byte_size);

		bool setIFN(size_t offset, const void * data, size_t len);

		void set(size_t offset, const void * data, size_t len);

		template <class T>
		void set(size_t index, T const& t)
		{
			const size_t offset = sizeof(T) * index;
			set(offset, &t, sizeof(T));
		}

		template <class T>
		
		bool setIFN(size_t index, T const& t)
		{
			const size_t offset = sizeof(T) * index;
			return setIFN(offset, &t, sizeof(T));
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

		BufferSegment getSegment() const
		{
			return BufferSegment{
				.buffer = _buffer,
				.range = Buffer::Range{.begin = 0, .len = _byte_size},
			};
		}

		BufferSegmentInstance getSegmentInstance() const
		{
			return BufferSegmentInstance{
				.buffer = _buffer->instance(),
				.range = Buffer::Range{.begin = 0, .len = _byte_size},
			};
		}
	};
}