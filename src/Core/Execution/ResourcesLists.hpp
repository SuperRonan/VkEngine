#pragma once

#include <vector>
#include <memory>

namespace vkl
{
	class ImageView;
	class Buffer;
	class Sampler;
	class DescriptorSetAndPool;
	class Command;
	class UpdateContext;

	struct ResourcesLists
	{
		std::vector<std::shared_ptr<ImageView>> images;
		std::vector<std::shared_ptr<Buffer>> buffers;
		std::vector<std::shared_ptr<Sampler>> samplers;
		std::vector<std::shared_ptr<DescriptorSetAndPool>> sets;
		std::vector<std::shared_ptr<Command>> commands;

		ResourcesLists& operator+=(ResourcesLists const& o);

		ResourcesLists& operator+=(std::shared_ptr<ImageView> const& image);
		ResourcesLists& operator+=(std::shared_ptr<Buffer> const& buffer);
		ResourcesLists& operator+=(std::shared_ptr<Sampler> const& sampler);
		ResourcesLists& operator+=(std::shared_ptr<DescriptorSetAndPool> const& set);
		ResourcesLists& operator+=(std::shared_ptr<Command> const& cmd);


		ResourcesLists operator+(ResourcesLists const& o) const;

		ResourcesLists operator+(std::shared_ptr<ImageView> const& image) const;
		ResourcesLists operator+(std::shared_ptr<Buffer> const& buffer) const;
		ResourcesLists operator+(std::shared_ptr<Sampler> const& sampler) const;
		ResourcesLists operator+(std::shared_ptr<DescriptorSetAndPool> const& set) const;
		ResourcesLists operator+(std::shared_ptr<Command> const& cmd) const;

		void update(UpdateContext& context);
	};
}