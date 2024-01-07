#include "ImageView.hpp"

namespace vkl
{
	std::atomic<size_t> ImageViewInstance::_instance_counter = 0;
	
	void ImageViewInstance::create()
	{
		_ci.image = *_image;
		VK_CHECK(vkCreateImageView(_app->device(), &_ci, nullptr, &_view), "Failed to create an image view.");

		setVkNameIFP();
	}

	void ImageViewInstance::setVkNameIFP()
	{
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT view_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
				.objectHandle = (uint64_t)_view,
				.pObjectName = name().c_str(),
			};
			_app->nameVkObjectIFP(view_name);
		}
	}

	void ImageViewInstance::destroy()
	{
		assert(!!_view);
		callDestructionCallbacks();
		vkDestroyImageView(_app->device(), _view, nullptr);
		_view = VK_NULL_HANDLE;
		_image = nullptr;
	}

	ImageViewInstance::ImageViewInstance(CreateInfo const& ci):
		AbstractInstance(ci.app, ci.name),
		_image(ci.image),
		_ci(ci.ci),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{
		create();
	}

	ImageViewInstance::~ImageViewInstance()
	{
		if (!!_view)
		{
			destroy();
		}
	}


	void ImageView::createInstance()
	{
		if (_inst)
		{
			destroyInstance();
		}
		VkImageViewCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = _type,
			.format = *_format,
			.components = _components,
			.subresourceRange = _range,
		};
		
		_inst = std::make_shared<ImageViewInstance>(ImageViewInstance::CI{
			.app = application(),
			.name = name(),
			.image = _image->instance(),
			.ci = ci,
		});
	}

	void ImageView::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	ImageView::~ImageView()
	{
		destroyInstance();
		_image->removeInvalidationCallbacks(this);
	}

	void ImageView::constructorBody(bool create_instance)
	{
		_image->addInvalidationCallback(Callback{
				.callback = [&]()
				{
					this->destroyInstance();
				},
				.id = this,
			});
		if (create_instance)
		{
			createInstance();
		}
	}

	ImageView::ImageView(CreateInfo const& ci) :
		InstanceHolder<ImageViewInstance>((ci.app ? ci.app : ci.image->application()), ci.name),
		_image(ci.image),
		_type(ci.type == VK_IMAGE_TYPE_MAX_ENUM ? getDefaultViewTypeFromImageType(_image->type()) : ci.type),
		_format(ci.format.hasValue() ? ci.format : _image->format()),
		_components(ci.components),
		_range(ci.range.has_value() ? ci.range.value() : _image->defaultSubresourceRange())
	{
		constructorBody(ci.create_on_construct);
	}

	ImageView::ImageView(Image::CreateInfo const& ci):
		InstanceHolder<ImageViewInstance>(ci.app, ci.name),
		_image(std::make_shared<Image>(ci)),
		_type(getDefaultViewTypeFromImageType(_image->type())),
		_format(_image->format()),
		_components(defaultComponentMapping()),
		_range(_image->defaultSubresourceRange())
	{
		constructorBody(ci.create_on_construct);
	}


	bool ImageView::updateResource(UpdateContext & ctx)
	{
		const bool updated = _image->updateResource(ctx);
		bool res = updated;
		
		if (_inst)
		{
			const VkFormat new_format = *_format;
			if (_inst->createInfo().format != new_format)
			{
				res = true;
			}

			if (res)
			{
				destroyInstance();
			}
		}

		if (!_inst)
		{
			createInstance();
			res = true;
		}

		return res;
	}

}