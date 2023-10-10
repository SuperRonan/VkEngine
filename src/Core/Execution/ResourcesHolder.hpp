#pragma once

#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/Sampler.hpp>

namespace vkl
{
	class Command;
	class DescriptorSetAndPool;

	struct ResourcesToUpload
	{
		struct ImageUpload
		{
			ObjectView src;
			std::shared_ptr<ImageView> dst;
		};

		std::vector<ImageUpload> images;

		struct BufferUpload
		{
			std::vector<PositionedObjectView> sources;
			std::shared_ptr<Buffer> dst;
		};
		
		std::vector<BufferUpload> buffers;

		ResourcesToUpload& operator+=(ResourcesToUpload const& o);

		ResourcesToUpload& operator+=(ImageUpload const& iu);

		ResourcesToUpload& operator+=(BufferUpload const& bu);


		ResourcesToUpload operator+(ResourcesToUpload const& o) const;

		ResourcesToUpload operator+(ImageUpload const& iu) const;

		ResourcesToUpload operator+(BufferUpload const& bu) const;
	};

	struct ResourcesToDeclare
	{
		std::vector<std::shared_ptr<ImageView>> images;
		std::vector<std::shared_ptr<Buffer>> buffers;
		std::vector<std::shared_ptr<Sampler>> samplers;
		std::vector<std::shared_ptr<DescriptorSetAndPool>> sets;
		std::vector<std::shared_ptr<Command>> commands;

		ResourcesToDeclare& operator+=(ResourcesToDeclare const& o);

		ResourcesToDeclare& operator+=(std::shared_ptr<ImageView> const& image);
		ResourcesToDeclare& operator+=(std::shared_ptr<Buffer> const& buffer);
		ResourcesToDeclare& operator+=(std::shared_ptr<Sampler> const& sampler);
		ResourcesToDeclare& operator+=(std::shared_ptr<DescriptorSetAndPool> const& set);
		ResourcesToDeclare& operator+=(std::shared_ptr<Command> const& cmd);


		ResourcesToDeclare operator+(ResourcesToDeclare const& o) const;

		ResourcesToDeclare operator+(std::shared_ptr<ImageView> const& image) const;
		ResourcesToDeclare operator+(std::shared_ptr<Buffer> const& buffer) const;
		ResourcesToDeclare operator+(std::shared_ptr<Sampler> const& sampler) const;
		ResourcesToDeclare operator+(std::shared_ptr<DescriptorSetAndPool> const& set) const;
		ResourcesToDeclare operator+(std::shared_ptr<Command> const& cmd) const;
	};

	class ResourcesHolder
	{
	public:

		virtual ResourcesToDeclare getResourcesToDeclare() = 0;

		virtual ResourcesToUpload getResourcesToUpload() = 0;

		virtual void notifyDataIsUploaded() = 0;
	};
}