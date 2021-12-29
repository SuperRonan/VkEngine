#pragma once

#include "VkApplication.hpp"
#include "Image.hpp"
#include <array>

namespace vkl
{
	class ImageView : public VkObject
	{
	public:

		struct DirectCreateInfo
		{

		};

		static constexpr VkComponentMapping defaultComponentMapping()
		{
			VkComponentMapping res(VK_COMPONENT_SWIZZLE_IDENTITY);
			return res;
		}

		static constexpr VkImageViewType getViewTypeFromImageType(VkImageType type)
		{
			VkImageViewType res;
			switch (type)
			{
			case(VK_IMAGE_TYPE_1D):
				res = VK_IMAGE_VIEW_TYPE_1D;
				break;
			case(VK_IMAGE_TYPE_2D):
				res = VK_IMAGE_VIEW_TYPE_2D;
				break;
			case(VK_IMAGE_TYPE_3D):
				res = VK_IMAGE_VIEW_TYPE_3D;
				break;
			default:
				throw std::logic_error("Unknow image type.");
				break;
			}
			return res;
		}

	protected:

		VkImage _image;
		VkImageViewType _type;
		VkFormat _format;
		VkComponentMapping _components;
		VkImageSubresourceRange _range;

		VkImageView _view = VK_NULL_HANDLE;


	public:

		constexpr ImageView(VkApplication * app = nullptr) noexcept :
			VkObject(app)
		{}

		ImageView(Image const& image, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

		ImageView(ImageView const&) = delete;

		constexpr ImageView(ImageView&& other) noexcept:
			VkObject(std::move(other)),
			_image(other._image),
			_type(other._type),
			_format(other._format),
			_components(other._components),
			_range(other._range),
			_view(other._view)
		{
			other._view = VK_NULL_HANDLE;
		}

		ImageView& operator=(ImageView const&) = delete;

		constexpr ImageView& operator=(ImageView&& other)
		{
			std::copySwap(*this, other);
			return *this;
		}

		~ImageView();

		void createView();

		void destroyView();

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
	};
}