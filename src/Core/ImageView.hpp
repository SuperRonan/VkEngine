#pragma once

#include "VkApplication.hpp"
#include "Image.hpp"
#include <array>
#include <optional>

namespace vkl
{
	class ImageViewInstance : public VkObject
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageInstance> image = nullptr;
			VkImageViewCreateInfo ci = {};
		};
		using CI = CreateInfo;

	protected:
		
		std::shared_ptr<ImageInstance> _image = nullptr;

		VkImageViewCreateInfo _ci = {};

		VkImageView _view = VK_NULL_HANDLE;

		void create();

		void destroy();

		void setVkNameIFP();

	public:

		ImageViewInstance() = delete;


		ImageViewInstance(ImageViewInstance const&) = delete;
		ImageViewInstance(ImageViewInstance &&) = delete;

		ImageViewInstance& operator=(ImageViewInstance const&) = delete;
		ImageViewInstance& operator=(ImageViewInstance &&) = delete;

		ImageViewInstance(CreateInfo const& ci);

		virtual ~ImageViewInstance();

		constexpr VkImageView view()const
		{
			return _view;
		}

		constexpr auto handle()const
		{
			return view();
		}

		constexpr operator VkImageView()const
		{
			return view();
		}

		auto image()const
		{
			return _image;
		}

		constexpr const VkImageViewCreateInfo& createInfo()const
		{
			return _ci;
		}
		 
	};

	class ImageView : public AbstractInstanceHolder
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = "";
			std::shared_ptr<Image> image = nullptr;
			VkImageViewType type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
			VkFormat format = VK_FORMAT_MAX_ENUM;
			VkComponentMapping components = defaultComponentMapping();
			std::optional<VkImageSubresourceRange> range = {};
			bool create_on_construct = false;
		};

		using CI = CreateInfo;


	protected:

		std::shared_ptr<Image> _image = nullptr;
		VkImageViewType _type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		VkFormat _format = VK_FORMAT_MAX_ENUM;
		VkComponentMapping _components = defaultComponentMapping();
		VkImageSubresourceRange _range = {};

		std::shared_ptr<ImageViewInstance> _inst = nullptr;

	public:

		constexpr ImageView(VkApplication * app = nullptr) noexcept :
			AbstractInstanceHolder(app, ""sv)
		{}

		//ImageView(std::shared_ptr<Image> image, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
		//ImageView(Image && image, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

		ImageView(CreateInfo const& ci);

		virtual ~ImageView() override;

		void createInstance();

		void destroyInstance();

		std::shared_ptr<ImageViewInstance> instance() const
		{
			return _inst;
		}

		constexpr const auto& image()const
		{
			return _image;
		}

		constexpr VkImageViewType type()const
		{
			return _type;
		}

		constexpr VkFormat format()const
		{
			return _format;
		}

		constexpr VkComponentMapping components()const
		{
			return _components;
		}

		constexpr VkImageSubresourceRange range()const
		{
			return _range;
		}

		bool updateResource(UpdateContext & ctx);

		// StagingPool::StagingBuffer* copyToStaging2D(StagingPool& pool, void* data, uint32_t elem_size);
		
		// void recordSendStagingToDevice2D(VkCommandBuffer command_buffer, StagingPool::StagingBuffer* sb, VkImageLayout layout);
		
		//void recordTransitionLayout(VkCommandBuffer command, VkImageLayout src, VkAccessFlags src_access, VkPipelineStageFlags src_stage, VkImageLayout dst, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage);
		
	};
}