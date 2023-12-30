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

			if (!_host_image.empty())
			{
				if (_desired_format.vk_format == VK_FORMAT_MAX_ENUM)
				{
					_desired_format = _host_image.format();
					_desired_format.determineVkFormatFromInfo();
				}

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
				.mips = Image::ALL_MIPS,
				.usage = _image_usages,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});

			_all_mips_view = std::make_shared<ImageView>(ImageView::CI{
				.app = application(),
				.name = name() + "_all_mip"s,
				.image = _image,
			});

			_top_mip_view = std::make_shared<ImageView>(ImageView::CI{
				.app = application(),
				.name = name() + "_top_mip"s,
				.image = _image,
				.range = VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = _image->layers(),
				},
			});

			_should_upload = true;
		}
	}

	void TextureFromFile::launchLoadTask()
	{
		_load_image_task = std::make_shared<AsynchTask>(AsynchTask::CI{
			.name = name() + ".loadHostImage()",
			.priority = TaskPriority::WhenPossible(),
			.lambda = [this]() {
				//std::this_thread::sleep_for(5s);
				loadHostImage();
				createDeviceImage();

				AsynchTask::ReturnType res{
					.success = true,
				};
				return res;
			},
		});
		application()->threadPool().pushTask(_load_image_task);
	}

	TextureFromFile::TextureFromFile(CreateInfo const& ci):
		Texture(Texture::CI{
			.app = ci.app,
			.name = ci.name,
		}),
		_path(ci.path),
		_is_synch(ci.synch)
	{
		if (ci.desired_format.has_value())
		{
			_desired_format = ci.desired_format.value();
		}


		if (_is_synch)
		{
			loadHostImage();
			createDeviceImage();
		}
		else
		{
			launchLoadTask();
		}
	}

	TextureFromFile::~TextureFromFile()
	{
		if (_load_image_task)
		{
			_load_image_task->cancel(false);
			_load_image_task->wait();
		}
	}

	void TextureFromFile::updateResources(UpdateContext& ctx)
	{
		
		if (ctx.updateTick() <= _latest_update_tick)
		{
			return;
		}

		_latest_update_tick = ctx.updateTick();

		if (_image)
		{
			_image->updateResource(ctx);
			_top_mip_view->updateResource(ctx);
			_all_mips_view->updateResource(ctx);
		}

		
		if (_should_upload)
		{
			_should_upload = false;
			bool synch_upload = _is_synch;
			if (!synch_upload && ctx.uploadQueue() == nullptr)
			{
				synch_upload = true;
			}
			if (synch_upload)
			{
				ResourcesToUpload::ImageUpload up{
					.src = ObjectView(_host_image.rawData(), _host_image.byteSize()),
					.dst = _top_mip_view->instance(),
				};
				ctx.resourcesToUpload() += std::move(up);
				_upload_done = true;
			}
			else
			{
				if (_load_image_task)
				{
					if (_load_image_task->StatusIsFinish(_load_image_task->getStatus()))
					{
						if (_load_image_task->isSuccess())
						{
							_upload_done = false;
							AsynchUpload up{
								.name = name(),
								.source = ObjectView(_host_image.rawData(), _host_image.byteSize()),
								.target_view = _top_mip_view->instance(),
								.completion_callback = [this](int ret)
								{
									if (ret == 0)
									{
										_upload_done = true;
									}
								},
							};
							ctx.uploadQueue()->enqueue(up);
						}
						else
						{
							_should_upload = false;
							_view = nullptr;
							_image = nullptr;
							_top_mip_view = nullptr;
							_all_mips_view = nullptr;
						}
						_load_image_task = nullptr;
					}
				}
			}
		}
		

		
		if (_upload_done)
		{
			_view = _top_mip_view;
			_is_ready = true;
			_upload_done = false;
			callResourceUpdateCallbacks();

			if (_image->instance()->createInfo().mipLevels > 1)
			{
				_mips_done = false;
				ctx.mipsQueue()->enqueue(AsynchMipsCompute{
					.target = _all_mips_view->instance(),
					.completion_callback = [this](int ret)
					{
						if (ret == 0)
						{
							_mips_done = true;
						}
					},
				});
			}
		}
		

		if (_mips_done)
		{
			_view = _all_mips_view;
			_is_ready = true;
			_mips_done = false;
			callResourceUpdateCallbacks();
		}
	}



}