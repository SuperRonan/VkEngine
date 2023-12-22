#include "ResourcesLists.hpp"

#include <Core/VulkanCommons.hpp>

#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/Sampler.hpp>
#include <Core/Commands/Command.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>

namespace vkl
{
	void ResourcesLists::update(UpdateContext& context)
	{
		for (auto& image_view : images)
		{
			assert(!!image_view);
			const bool invalidated = image_view->updateResource(context);
		}

		for (auto& buffer : buffers)
		{
			assert(!!buffer);
			const bool invalidated = buffer->updateResource(context);
		}

		for (auto& sampler : samplers)
		{
			assert(!!sampler);
			const bool invalidated = sampler->updateResources(context);
		}

		for (auto& command : commands)
		{
			assert(!!command);
			const bool invalidated = command->updateResources(context);
		}

		for (auto& set : sets)
		{
			assert(!!set);
			const bool invalidated = set->updateResources(context);
		}
	}



	ResourcesLists& ResourcesLists::operator+=(ResourcesLists const& o)
	{
		using namespace std::containers_append_operators;
		images += o.images;
		buffers += o.buffers;
		samplers += o.samplers;
		sets += o.sets;
		commands += o.commands;
		return *this;
	}

	ResourcesLists& ResourcesLists::operator+=(std::shared_ptr<ImageView> const& image)
	{
		images.push_back(image);
		return *this;
	}
	ResourcesLists& ResourcesLists::operator+=(std::shared_ptr<Buffer> const& buffer)
	{
		buffers.push_back(buffer);
		return *this;
	}
	ResourcesLists& ResourcesLists::operator+=(std::shared_ptr<Sampler> const& sampler)
	{
		samplers.push_back(sampler);
		return *this;
	}
	ResourcesLists& ResourcesLists::operator+=(std::shared_ptr<DescriptorSetAndPool> const& set)
	{
		sets.push_back(set);
		return *this;
	}
	ResourcesLists& ResourcesLists::operator+=(std::shared_ptr<Command> const& cmd)
	{
		commands.push_back(cmd);
		return *this;
	}


	ResourcesLists ResourcesLists::operator+(ResourcesLists const& o) const
	{
		ResourcesLists res = *this;
		res += o;
		return res;
	}

	ResourcesLists ResourcesLists::operator+(std::shared_ptr<ImageView> const& image) const
	{
		ResourcesLists res = *this;
		res += image;
		return res;
	}
	ResourcesLists ResourcesLists::operator+(std::shared_ptr<Buffer> const& buffer) const
	{
		ResourcesLists res = *this;
		res += buffer;
		return res;
	}
	ResourcesLists ResourcesLists::operator+(std::shared_ptr<Sampler> const& sampler) const
	{
		ResourcesLists res = *this;
		res += sampler;
		return res;
	}
	ResourcesLists ResourcesLists::operator+(std::shared_ptr<DescriptorSetAndPool> const& set) const
	{
		ResourcesLists res = *this;
		res += set;
		return res;
	}
	ResourcesLists ResourcesLists::operator+(std::shared_ptr<Command> const& cmd) const
	{
		ResourcesLists res = *this;
		res += cmd;
		return res;
	}
}