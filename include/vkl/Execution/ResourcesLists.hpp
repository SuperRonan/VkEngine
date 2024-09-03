#pragma once

#include <memory>

#include <vkl/Utils/MyVector.hpp>

namespace vkl
{
	class ImageView;
	class Buffer;
	class Sampler;
	class DescriptorSetAndPool;
	class Command;
	class Framebuffer;
	class UpdateContext;

	struct ResourcesLists
	{
		MyVector<std::shared_ptr<ImageView>> images;
		MyVector<std::shared_ptr<Buffer>> buffers;
		MyVector<std::shared_ptr<Sampler>> samplers;
		MyVector<std::shared_ptr<DescriptorSetAndPool>> sets;
		MyVector<std::shared_ptr<Command>> commands;
		MyVector<std::shared_ptr<Framebuffer>> framebuffers;

		ResourcesLists& operator+=(ResourcesLists const& o);

		ResourcesLists& operator+=(std::shared_ptr<ImageView> const& image);
		ResourcesLists& operator+=(std::shared_ptr<Buffer> const& buffer);
		ResourcesLists& operator+=(std::shared_ptr<Sampler> const& sampler);
		ResourcesLists& operator+=(std::shared_ptr<DescriptorSetAndPool> const& set);
		ResourcesLists& operator+=(std::shared_ptr<Command> const& cmd);
		ResourcesLists& operator+=(std::shared_ptr<Framebuffer> const& fb);


		ResourcesLists operator+(ResourcesLists const& o) const;

		ResourcesLists operator+(std::shared_ptr<ImageView> const& image) const;
		ResourcesLists operator+(std::shared_ptr<Buffer> const& buffer) const;
		ResourcesLists operator+(std::shared_ptr<Sampler> const& sampler) const;
		ResourcesLists operator+(std::shared_ptr<DescriptorSetAndPool> const& set) const;
		ResourcesLists operator+(std::shared_ptr<Command> const& cmd) const;

		void update(UpdateContext& context);

		void clear();
	};
}