#include "TextureFromFile.hpp"
#include <thatlib/src/img/ImRead.hpp>

namespace vkl
{
	

	DetailedVkFormat TextureFromFile::findFormatForVkImage(DetailedVkFormat const& f)
	{
		VkImageFormatProperties props;
		DetailedVkFormat res = f;
		while (true)
		{
			const VkResult vk_res = vkGetPhysicalDeviceImageFormatProperties(
				application()->physicalDevice(),
				res.vk_format,
				VK_IMAGE_TYPE_2D,
				VK_IMAGE_TILING_OPTIMAL,
				_image_usages,
				VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
				&props);

			if (vk_res == VK_ERROR_FORMAT_NOT_SUPPORTED)
			{
				if (res.color.channels == 3)
				{
					res.color.channels = 4;
					res.color.bits[3] = res.color.bits[2];
					bool mutate = res.determineVkFormatFromInfo();
					assertm(mutate, "Could not mutate format");
					continue;
				}
				else
				{
					assertm(false, "Could not find a suitable VkImage format");
					break;
				}
			}
			else
			{
				assert(vk_res == VK_SUCCESS);
				break;
			}
		}
		
		
		return res;
	}

	TextureFromFile::TextureFromFile(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		ResourcesHolder(),
		_path(ci.path)
	{
		if (!_path.empty())
		{
			_host_image = img::io::readFormatedImage(_path);

			if (ci.desired_format.has_value())
			{
				_desired_format = ci.desired_format.value();
			}
			else
			{
				_desired_format = _host_image.format();
				_desired_format.determineVkFormatFromInfo();
			}

			if (!_host_image.empty())
			{

				_image_format = findFormatForVkImage(_desired_format);

				if (_image_format.vk_format != _desired_format.vk_format)
				{
					_host_image.reFormat(_image_format.getImgFormatInfo());
				}

				_image = std::make_shared<Image>(Image::CI{
					.app = application(),
					.name = name(),
					.type = VK_IMAGE_TYPE_2D,
					.format = _image_format.vk_format,
					.extent = VkExtent3D{.width = static_cast<uint32_t>(_host_image.width()), .height = static_cast<uint32_t>(_host_image.height()), .depth = 1},
					.usage = _image_usages,
					.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				});

				_image_view = std::make_shared<ImageView>(ImageView::CI{
					.app = application(),
					.name = name(),
					.image = _image,
				});

				_should_update = true;
			}
		}
	}

	ResourcesToDeclare TextureFromFile::getResourcesToDeclare()
	{
		ResourcesToDeclare res;
		if (_image_view)
		{
			res += _image_view;
		}
		return res;
	}

	ResourcesToUpload TextureFromFile::getResourcesToUpload()
	{
		ResourcesToUpload res;
		if (_should_update && !!_image)
		{
			res += ResourcesToUpload::ImageUpload{
				.src = ObjectView(_host_image.rawData(), _host_image.byteSize()),
				.dst = _image_view,
			};
		}
		return res;
	}

	void TextureFromFile::notifyDataIsUploaded()
	{
		_should_update = false;
	}
}