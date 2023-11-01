#pragma once

#include <Core/VkObjects/ImageView.hpp>
#include <filesystem>
#include <thatlib/src/img/Image.hpp>
#include <Core/Execution/ResourcesHolder.hpp>
#include <Core/VkObjects/DetailedVkFormat.hpp>

namespace vkl
{
	class TextureFromFile : public VkObject, public ResourcesHolder
	{
	protected:

		std::filesystem::path _path = {};

		bool _is_synch = true;
		
		bool _should_update = false;
		img::FormatedImage _host_image = {};

		DetailedVkFormat _desired_format;
		DetailedVkFormat _image_format;

		VkImageUsageFlags _image_usages = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_SAMPLED_BIT;

		std::shared_ptr<Image> _image = nullptr;
		std::shared_ptr<ImageView> _image_view = nullptr;


		std::vector<Callback> _resource_update_callback = {};

		DetailedVkFormat findFormatForVkImage(DetailedVkFormat const& f);

		void loadHostImage();

		void createDeviceImage();

		void callResourceUpdateCallbacks();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::filesystem::path path = {};
			std::optional<VkFormat> desired_format = {};
			bool synch = true;
		};
		using CI = CreateInfo;

		TextureFromFile(CreateInfo const& ci);

		bool empty() const
		{
			return !_image_view;
		}

		bool hasValue() const
		{
			 return _image_view.operator bool();
		}

		const std::shared_ptr<ImageView>& view()const
		{
			return _image_view;
		}

		virtual void updateResources(UpdateContext& ctx);

		virtual ResourcesToUpload getResourcesToUpload() override;

		void addResourceUpdateCallback(Callback const& cb);

		void removeResourceUpdateCallback(VkObject * id);
	};
}