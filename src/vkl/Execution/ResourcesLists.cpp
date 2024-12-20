#include <vkl/Execution/ResourcesLists.hpp>

#include <vkl/Core/VulkanCommons.hpp>

#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/Buffer.hpp>
#include <vkl/VkObjects/Sampler.hpp>
#include <vkl/Commands/Command.hpp>
#include <vkl/Execution/DescriptorSetsManager.hpp>
#include <vkl/VkObjects/Framebuffer.hpp>

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

		for (auto& fb : framebuffers)
		{
			assert(fb);
			const bool invalidated = fb->updateResources(context);
		}
	}

	void ResourcesLists::clear()
	{
		images.clear();
		buffers.clear();
		samplers.clear();
		commands.clear();
		sets.clear();
		framebuffers.clear();
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
	ResourcesLists& ResourcesLists::operator+=(std::shared_ptr<Framebuffer> const& fb)
	{
		framebuffers.push_back(fb);
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