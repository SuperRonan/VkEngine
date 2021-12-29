#include "ImageView.hpp"

namespace vkl
{
	void ImageView::createView()
	{
		VkImageViewCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = _image,
			.viewType = _type,
			.format = _format,
			.components = _components,
			.subresourceRange = _range,
		};

		VK_CHECK(vkCreateImageView(_app->device(), &ci, nullptr, &_view), "Failed to create an image view.");
	}

	void ImageView::destroyView()
	{
		vkDestroyImageView(_app->device(), _view, nullptr);
		_view = VK_NULL_HANDLE;
	}

	ImageView::~ImageView()
	{
		if (handle() != VK_NULL_HANDLE)
		{
			destroyView();
		}
	}

	ImageView::ImageView(Image const& image, VkImageAspectFlags aspect) :
		VkObject(image),
		_image(image.handle()),
		_type(getViewTypeFromImageType(image.type())),
		_format(image.format()),
		_components(defaultComponentMapping()),
		_range(VkImageSubresourceRange{
			.aspectMask = aspect,
			.baseMipLevel = 0,
			.levelCount = image.mips(),
			.baseArrayLayer = 0,
			.layerCount = 1,
		})
		{
			createView();
		}


}