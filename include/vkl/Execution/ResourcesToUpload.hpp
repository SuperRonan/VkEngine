#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/Execution/CompletionCallback.hpp>
#include <that/utils/ExtensibleDataStorage.hpp>
#include <that/utils/ExtensibleStorage.hpp>

namespace vkl
{
	
	class ImageViewInstance;
	class BufferInstance;
	
	struct ResourcesToUpload
	{
	protected:
		friend struct ResourcesToUploadTemplateProcessor;
	public:

		that::ExDS data;
		

		struct ImageUpload
		{
			union
			{
				const void * data = nullptr;
				uintptr_t data_begin;
			};
			size_t size = 0;
			bool copy_data = false;
			// 0 -> tightly packed
			uint32_t buffer_row_length = 0;
			uint32_t buffer_image_height = 0;
			std::shared_ptr<ImageViewInstance> dst = nullptr;
			CompletionCallback completion_callback = {};
		};

		MyVector<ImageUpload> images;

		// Don't like the vector in vector
		struct BufferUpload
		{
			struct Source
			{
				union
				{
					const void * data = nullptr;
					uintptr_t data_begin;
				};
				size_t size = 0;
				size_t offset = 0;
				bool copy_data = false;
			};
			union
			{
				const Source * sources = nullptr;
				uintptr_t sources_begin;
			};
			size_t sources_count = 0;
			std::shared_ptr<BufferInstance> dst = nullptr;
			CompletionCallback completion_callback = {};
		};
		using BufferSource = BufferUpload::Source;

		that::ExS<BufferSource> buffer_sources;
		MyVector<BufferUpload> buffers;

		ResourcesToUpload() noexcept = default;

		ResourcesToUpload(ResourcesToUpload const&) = default;
		ResourcesToUpload(ResourcesToUpload &&) noexcept = default;

		ResourcesToUpload& operator=(ResourcesToUpload const&) = default;
		ResourcesToUpload& operator=(ResourcesToUpload &&) noexcept = default;


		ResourcesToUpload& operator+=(ResourcesToUpload const& o);
		ResourcesToUpload& operator+=(ResourcesToUpload && o);

		ResourcesToUpload& operator+=(ImageUpload const& iu);
		ResourcesToUpload& operator+=(ImageUpload && iu);

		ResourcesToUpload& operator+=(BufferUpload const& bu);
		ResourcesToUpload& operator+=(BufferUpload && bu);

		
		size_t getSize()const;

		void clear();

		template <class UploadInfo>
		const void* getSrcData(UploadInfo const& info) const
		{
			const void * res = info.data;
			if (info.copy_data)
			{
				const uintptr_t index = info.data_begin;
				res = data.data() + index;
			}
			return res;
		}

		void offsetImagesData(size_t begin, size_t count, uintptr_t offset);

		void offsetBuffersData(size_t begin, size_t count, uintptr_t data_offset, size_t buffer_source_offset);
	};
}