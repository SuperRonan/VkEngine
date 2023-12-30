#include "ResourcesToUpload.hpp"
#include <Core/Utils/stl_extension.hpp>

namespace vkl
{
	ResourcesToUpload& ResourcesToUpload::operator+=(ResourcesToUpload const& o)
	{
		using namespace std::containers_append_operators;

		images += o.images;
		buffers += o.buffers;
		return *this;
	}

	ResourcesToUpload& ResourcesToUpload::operator+=(ImageUpload const& iu)
	{
		images.push_back(iu);
		return *this;
	}

	ResourcesToUpload& ResourcesToUpload::operator+=(BufferUpload const& bu)
	{
		buffers.push_back(bu);
		return *this;
	}

	ResourcesToUpload& ResourcesToUpload::operator+=(ResourcesToUpload && o)
	{
		using namespace std::containers_append_operators;

		images += std::move(o.images);
		buffers += std::move(o.buffers);
		return *this;
	}

	ResourcesToUpload& ResourcesToUpload::operator+=(ImageUpload && iu)
	{
		images.emplace_back(std::move(iu));
		return *this;
	}

	ResourcesToUpload& ResourcesToUpload::operator+=(BufferUpload && bu)
	{
		buffers.emplace_back(std::move(bu));
		return *this;
	}


	ResourcesToUpload ResourcesToUpload::operator+(ResourcesToUpload const& o) const
	{
		ResourcesToUpload res = *this;
		res += o;
		return res;
	}

	ResourcesToUpload ResourcesToUpload::operator+(ImageUpload const& iu) const
	{
		ResourcesToUpload res = *this;
		res += iu;
		return res;
	}

	ResourcesToUpload ResourcesToUpload::operator+(BufferUpload const& bu) const
	{
		ResourcesToUpload res = *this;
		res += bu;
		return res;
	}

	size_t ResourcesToUpload::getSize() const
	{
		size_t res = 0;
		for (const auto& b : buffers)
		{
			for (const auto& s : b.sources)
			{
				res += s.obj.size();
			}
		}
		for (const auto& i : images)
		{
			res += i.src.size();
		}
		return res;
	}

	void ResourcesToUpload::clear()
	{
		buffers.clear();
		images.clear();
	}
}