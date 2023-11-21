#include "TextureFromFile.hpp"
#include <thatlib/src/img/ImRead.hpp>

#include <chrono>

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

	void TextureFromFile::loadHostImage()
	{
		if (!_path.empty())
		{
			_host_image = img::io::readFormatedImage(_path);

			if (_desired_format.vk_format == VK_FORMAT_MAX_ENUM)
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
			}
		}
	}

	void TextureFromFile::createDeviceImage()
	{
		if (!_host_image.empty())
		{
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

	TextureFromFile::TextureFromFile(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		ResourcesHolder(),
		_path(ci.path),
		_is_synch(ci.synch)
	{
		assert(_is_synch);
		if (ci.desired_format.has_value())
		{
			_desired_format = ci.desired_format.value();
		}


		if (_is_synch)
		{
			loadHostImage();
			createDeviceImage();
		}
	}

	void TextureFromFile::updateResources(UpdateContext& ctx)
	{

		if (_image_view)
		{
			_image_view->updateResource(ctx);
		}

		if (_is_synch)
		{
			if (_should_update && !!_image)
			{
				ctx.resourcesToUpload() += ResourcesToUpload::ImageUpload{
					.src = ObjectView(_host_image.rawData(), _host_image.byteSize()),
					.dst = _image_view,
				};
				_should_update = false;
			}
		}
		else
		{

		}
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
			_should_update = false;
		}
		return res;
	}

	void TextureFromFile::addResourceUpdateCallback(Callback const& cb)
	{
		_resource_update_callback.push_back(cb);
	}

	void TextureFromFile::removeResourceUpdateCallback(VkObject* id)
	{
		for (size_t i = 0; i < _resource_update_callback.size(); ++i)
		{
			if (_resource_update_callback[i].id == id)
			{
				_resource_update_callback.erase(_resource_update_callback.begin() + i);
				break;
			}
		}
	}

	void TextureFromFile::callResourceUpdateCallbacks()
	{
		for (size_t i = 0; i < _resource_update_callback.size(); ++i)
		{
			_resource_update_callback[i].callback();
		}
	}

}