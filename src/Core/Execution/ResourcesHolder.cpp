#include "ResourcesHolder.hpp"

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



	ResourcesToDeclare& ResourcesToDeclare::operator+=(ResourcesToDeclare const& o)
	{
		using namespace std::containers_operators;
		images += o.images;
		buffers += o.buffers;
		samplers += o.samplers;
		sets += o.sets;
		commands += o.commands;
		return *this;
	}

	ResourcesToDeclare& ResourcesToDeclare::operator+=(std::shared_ptr<ImageView> const& image)
	{
		images.push_back(image);
		return *this;
	}
	ResourcesToDeclare& ResourcesToDeclare::operator+=(std::shared_ptr<Buffer> const& buffer)
	{
		buffers.push_back(buffer);
		return *this;
	}
	ResourcesToDeclare& ResourcesToDeclare::operator+=(std::shared_ptr<Sampler> const& sampler)
	{
		samplers.push_back(sampler);
		return *this;
	}
	ResourcesToDeclare& ResourcesToDeclare::operator+=(std::shared_ptr<DescriptorSetAndPool> const& set)
	{
		sets.push_back(set);
		return *this;
	}
	ResourcesToDeclare& ResourcesToDeclare::operator+=(std::shared_ptr<Command> const& cmd)
	{
		commands.push_back(cmd);
		return *this;
	}


	ResourcesToDeclare ResourcesToDeclare::operator+(ResourcesToDeclare const& o) const
	{
		ResourcesToDeclare res = *this;
		res += o;
		return res;
	}

	ResourcesToDeclare ResourcesToDeclare::operator+(std::shared_ptr<ImageView> const& image) const
	{
		ResourcesToDeclare res = *this;
		res += image;
		return res;
	}
	ResourcesToDeclare ResourcesToDeclare::operator+(std::shared_ptr<Buffer> const& buffer) const
	{
		ResourcesToDeclare res = *this;
		res += buffer;
		return res;
	}
	ResourcesToDeclare ResourcesToDeclare::operator+(std::shared_ptr<Sampler> const& sampler) const
	{
		ResourcesToDeclare res = *this;
		res += sampler;
		return res;
	}
	ResourcesToDeclare ResourcesToDeclare::operator+(std::shared_ptr<DescriptorSetAndPool> const& set) const
	{
		ResourcesToDeclare res = *this;
		res += set;
		return res;
	}
	ResourcesToDeclare ResourcesToDeclare::operator+(std::shared_ptr<Command> const& cmd) const
	{
		ResourcesToDeclare res = *this;
		res += cmd;
		return res;
	}
}