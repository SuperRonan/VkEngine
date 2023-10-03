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
	};

	struct ResourcesToDeclare
	{
		std::vector<std::shared_ptr<ImageView>> images;
		std::vector<std::shared_ptr<Buffer>> buffers;
		std::vector<std::shared_ptr<Sampler>> samplers;
		std::vector<std::shared_ptr<DescriptorSetAndPool>> sets;
		std::vector<std::shared_ptr<Command>> commands;
	};

	class ResourcesHolder
	{
	public:

		virtual ResourcesToDeclare getResourcesToDeclare() = 0;

		virtual ResourcesToUpload getResourcesToUpload() = 0;

		virtual void notifyDataIsUploaded() = 0;
	};
}