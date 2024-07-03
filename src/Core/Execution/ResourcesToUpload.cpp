#include "ResourcesToUpload.hpp"
#include <Core/Utils/stl_extension.hpp>

#include <thatlib/src/core/Concepts.hpp>
#include <thatlib/src/stl_ext/const_forward.hpp>

namespace vkl
{
	struct ResourcesToUploadTemplateProcessor
	{
		template <that::concepts::UniversalReference<ResourcesToUpload> RTURef>
		static void Append_RTU(ResourcesToUpload& that, RTURef&& o)
		{
			using namespace std::containers_append_operators;
			const size_t data_base = that.data.pushBack(o.data.data(), o.data.size());
			const size_t bs_base = that.buffer_sources.pushBack(o.buffer_sources.data(), o.buffer_sources.size());

			const size_t old_image_size = that.images.size();
			const size_t old_buffer_size = that.buffers.size();

			that.images += std::forward<MyVector<ResourcesToUpload::ImageUpload>>(o.images);
			that.offsetImagesData(old_image_size, o.images.size(), data_base);

			that.buffers += std::forward<MyVector<ResourcesToUpload::BufferUpload>>(o.buffers);
			const size_t old_buffer_sources_size = that.buffer_sources.pushBack(o.buffer_sources.data(), o.buffer_sources.size());
			that.offsetBuffersData(old_buffer_size, o.buffers.size(), data_base, old_buffer_sources_size);
		}

		template <that::concepts::UniversalReference<ResourcesToUpload::ImageUpload> IURef>
		static void Append_image(ResourcesToUpload& that, IURef&& iu)
		{
			that.images.push_back(std::forward<ResourcesToUpload::ImageUpload>(iu));
			ResourcesToUpload::ImageUpload & _iu = that.images.back();
			if (_iu.copy_data)
			{
				uintptr_t & index = _iu.data_begin;
				index = that.data.pushBack(_iu.data, _iu.size);
			}
		}

		template <that::concepts::UniversalReference<ResourcesToUpload::BufferUpload> BURef>
		static void Append_buffer(ResourcesToUpload& that, BURef&& bu)
		{
			that.buffers.push_back(std::forward<ResourcesToUpload::BufferUpload>(bu));
			ResourcesToUpload::BufferUpload & _bu = that.buffers.back();
			uintptr_t & base = _bu.sources_begin;
			base = that.buffer_sources.pushBack(_bu.sources, _bu.sources_count);

			for (size_t i = 0; i < _bu.sources_count; ++i)
			{
				ResourcesToUpload::BufferSource & _bs = that.buffer_sources.data()[base + i];
				if (_bs.copy_data)
				{
					uintptr_t& index = _bs.data_begin;
					index = that.data.pushBack(_bs.data, _bs.size);
				}
			}
		}
	};

	void ResourcesToUpload::offsetImagesData(size_t begin, size_t count, uintptr_t offset)
	{
		for (size_t i = 0; i < count; ++i)
		{
			ImageUpload& iu = images[begin + i];
			if (iu.copy_data)
			{
				uintptr_t& index = iu.data_begin;
				index = offset + index;
			}
		}
	}

	void ResourcesToUpload::offsetBuffersData(size_t begin, size_t count, uintptr_t data_offset, size_t buffer_source_offset)
	{
		for (size_t i = 0; i < count; ++i)
		{
			BufferUpload & bu = buffers[begin + i];

			uintptr_t & sources_begin = bu.sources_begin;
			sources_begin += buffer_source_offset;

			for (size_t j = 0; j < bu.sources_count; ++j)
			{
				BufferSource & bs = buffer_sources.data()[sources_begin + j];
				if (bs.copy_data)
				{
					uintptr_t & index = bs.data_begin;
					index += data_offset;
				}
			}
		}
	}

	ResourcesToUpload& ResourcesToUpload::operator+=(ResourcesToUpload const& o)
	{
		ResourcesToUploadTemplateProcessor::Append_RTU<ResourcesToUpload const&>(*this, o);
		return *this;
	}

	ResourcesToUpload& ResourcesToUpload::operator+=(ResourcesToUpload && o)
	{
		ResourcesToUploadTemplateProcessor::Append_RTU<ResourcesToUpload &&>(*this, std::move(o));
		o.clear();
		return *this;
	}

	ResourcesToUpload& ResourcesToUpload::operator+=(ImageUpload const& iu)
	{
		ResourcesToUploadTemplateProcessor::Append_image<ImageUpload const&>(*this, iu);
		return *this;
	}
	
	ResourcesToUpload& ResourcesToUpload::operator+=(ImageUpload && iu)
	{
		ResourcesToUploadTemplateProcessor::Append_image<ImageUpload &&>(*this, std::move(iu));
		return *this;
	}

	
	ResourcesToUpload& ResourcesToUpload::operator+=(BufferUpload const& bu)
	{
		ResourcesToUploadTemplateProcessor::Append_buffer<BufferUpload const&>(*this, bu);
		return *this;
	}

	ResourcesToUpload& ResourcesToUpload::operator+=(BufferUpload && bu)
	{
		ResourcesToUploadTemplateProcessor::Append_buffer<BufferUpload &&>(*this, std::move(bu));
		return *this;
	}

	size_t ResourcesToUpload::getSize() const
	{
		size_t res = 0;
		for (const auto& b : buffers)
		{
			for (size_t i = 0; i < b.sources_count; ++i)
			{
				res += buffer_sources.data()[b.sources_begin + i].size;
			}
		}
		for (const auto& i : images)
		{
			res += i.size;
		}
		return res;
	}

	void ResourcesToUpload::clear()
	{
		data.clear();
		images.clear();
		buffer_sources.clear();
		buffers.clear();
	}
}