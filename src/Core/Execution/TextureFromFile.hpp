#pragma once

#include <Core/VkObjects/ImageView.hpp>
#include <filesystem>
#include <thatlib/src/img/Image.hpp>
#include <Core/Execution/ResourcesHolder.hpp>
#include <Core/VkObjects/DetailedVkFormat.hpp>

namespace vkl
{
	class TextureFromFile : public VkObject
	{
	protected:

		std::filesystem::path _path = {};

		bool _is_synch = true;
		
		bool _should_upload = false;
		bool _upload_done = false;
		bool _is_ready = false;
		img::FormatedImage _host_image = {};

		std::shared_ptr<AsynchTask> _load_image_task = nullptr;

		DetailedVkFormat _desired_format;
		DetailedVkFormat _image_format;

		VkImageUsageFlags _image_usages = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_SAMPLED_BIT;

		std::shared_ptr<Image> _image = nullptr;
		std::shared_ptr<ImageView> _image_view = nullptr;


		std::vector<Callback> _resource_update_callback = {};

		DetailedVkFormat findFormatForVkImage(DetailedVkFormat const& f);

		void loadHostImage();

		void createDeviceImage();

		void launchLoadTask();

		void callResourceUpdateCallbacks();

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

		bool empty() const
		{
			return !_image_view;
		}

		bool hasValue() const
		{
			 return _image_view.operator bool();
		}

		bool isReady() const
	 	{
			return _is_ready;	
		}

		const std::shared_ptr<ImageView>& view()const
		{
			return _image_view;
		}

		virtual void updateResources(UpdateContext& ctx);

		void addResourceUpdateCallback(Callback const& cb);

		void removeResourceUpdateCallback(VkObject * id);
	};
}