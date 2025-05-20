#pragma once

#include "Texture.hpp"
#include <filesystem>
#include <that/img/Image.hpp>

#include <unordered_map>
#include <mutex>

namespace vkl
{
	class TextureFromFile : public Texture
	{
	protected:

		size_t _latest_update_tick = 0;

		std::filesystem::path _path = {};

		bool _is_synch = true;
		
		bool _should_upload = false;
		bool _upload_done = false;
		bool _mips_done = false;
		
		that::img::FormatedImage _host_image = {};

		std::shared_ptr<AsynchTask> _load_image_task = nullptr;

		MipsOptions _desired_mips;
		uint32_t _desired_layers;

		DetailedVkFormat _desired_format;
		DetailedVkFormat _image_format;

		VkImageUsageFlags _image_usages = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_SAMPLED_BIT;

		std::shared_ptr<Image> _image = nullptr;
		std::shared_ptr<ImageView> _top_mip_view = nullptr;
		std::shared_ptr<ImageView> _all_mips_view = nullptr;

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
			VkFormat desired_format = VK_FORMAT_UNDEFINED;
			bool synch = false;
			MipsOptions mips = MipsOptions::Auto;
			uint32_t layers = 1;
		};
		using CI = CreateInfo;

		TextureFromFile(CreateInfo const& ci);

		virtual ~TextureFromFile() override;

		virtual void updateResources(UpdateContext& ctx) override;
	};

	class TextureFileCache : public VkObject
	{
	protected:

		std::mutex _mutex;
		
		std::unordered_map<std::filesystem::path, std::shared_ptr<TextureFromFile>> _cache;		

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		TextureFileCache(CreateInfo const& ci);


		std::shared_ptr<TextureFromFile> getTexture(std::filesystem::path const& path, VkFormat desired_format = VK_FORMAT_UNDEFINED);

		void updateResources(UpdateContext & ctx);
	};
}