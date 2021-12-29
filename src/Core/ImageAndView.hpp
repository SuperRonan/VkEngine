#pragma once

#include "VkApplication.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "StagingPool.hpp"

#include <memory>

namespace vkl
{
	class ImageAndView : public VkObject
	{
	public:

		struct CreateInfo : public Image::CreateInfo
		{
			VkImageAspectFlags aspect;
		};

	protected:

		std::shared_ptr<Image> _image = nullptr;
		std::shared_ptr<ImageView> _view = nullptr;

	public:

		constexpr ImageAndView(VkApplication * app = nullptr) noexcept : 
			VkObject(app)
		{}

		ImageAndView(ImageAndView const&) noexcept = default;

		ImageAndView(ImageAndView&&) noexcept = default;

		ImageAndView& operator=(ImageAndView const&) noexcept = default;

		ImageAndView& operator=(ImageAndView&&) noexcept = default;

		ImageAndView(VkApplication* app, const CreateInfo& ci);

		ImageAndView(std::shared_ptr<Image> const& image, std::shared_ptr<ImageView> const& view);

		ImageAndView(std::shared_ptr<Image> const& image, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

		void reset();

		constexpr auto& image()
		{
			return _image;
		}

		constexpr const auto& image()const
		{
			return _image;
		}

		constexpr auto& view()
		{
			return _view;
		}

		constexpr const auto& view()const
		{
			return _view;
		}

		StagingPool::StagingBuffer* copyToStaging2D(StagingPool& pool, void* data);

		void recordSendStagingToDevice2D(VkCommandBuffer command_buffer, StagingPool::StagingBuffer* sb, VkImageLayout layout);

		void recordTransitionLayout(VkCommandBuffer command, VkImageLayout src, VkAccessFlags src_access, VkPipelineStageFlags src_stage, VkImageLayout dst, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage);

	};
}