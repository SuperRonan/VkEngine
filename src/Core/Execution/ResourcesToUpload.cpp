#include "ResourcesToUpload.hpp"

namespace vkl
{
	ResourcesToUpload& ResourcesToUpload::operator+=(ResourcesToUpload const& o)
	{
		using namespace std::containers_operators;

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
}