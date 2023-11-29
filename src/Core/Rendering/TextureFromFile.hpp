#pragma once

#include "Texture.hpp"
#include <filesystem>
#include <thatlib/src/img/Image.hpp>

namespace vkl
{
	class TextureFromFile : public Texture
	{
	protected:

		std::filesystem::path _path = {};

		bool _is_synch = true;
		
		bool _should_upload = false;
		bool _upload_done = false;
		
		img::FormatedImage _host_image = {};

		std::shared_ptr<AsynchTask> _load_image_task = nullptr;

		DetailedVkFormat _desired_format;
		DetailedVkFormat _image_format;

		VkImageUsageFlags _image_usages = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_SAMPLED_BIT;

		std::shared_ptr<Image> _image = nullptr;

		DetailedVkFormat findFormatForVkImage(DetailedVkFormat const& f);

		void loadHostImage();

		void createDeviceImage();

		void launchLoadTask();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::filesystem::path path = {};
			std::optional<VkFormat> desired_format = {};
			bool synch = false;
		};
		using CI = CreateInfo;

		TextureFromFile(CreateInfo const& ci);

		virtual ~TextureFromFile() override;

		virtual void updateResources(UpdateContext& ctx) override;
	};
}