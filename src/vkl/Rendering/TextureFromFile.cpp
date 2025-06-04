#include <vkl/Rendering/TextureFromFile.hpp>
#include <vkl/IO/GuiContext.hpp>
#include <that/img/ImRead.hpp>

#include <chrono>

#include <imgui/misc/cpp/imgui_stdlib.h>

#include <vulkan/vk_enum_string_helper.h>

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
			that::img::io::ReadImageInfo read_info{
				.path = &_path,
				.filesystem = application()->fileSystem(),
				.target = &_host_image,
			};
			that::Result read_result = that::img::io::ReadFormatedImage(read_info);
			if (read_result != that::Result::Success)
			{
				application()->logger()(std::format("Could not read texture image: {}", _path.string()), Logger::Options::TagLowWarning);
				_host_image = {};
			}
			else
			{
				that::FormatInfo retarget_format = _host_image.format();
				
				if (_desired_format.vk_format != VK_FORMAT_UNDEFINED)
				{
					retarget_format.type = _desired_format.getImgFormatInfo().type;
				}
				else if (_host_image.format().type == that::ElementType::UNORM)
				{
					retarget_format.type = that::ElementType::sRGB;
				}
				if (retarget_format != _host_image.format())
				{
					that::FormatInfo new_format = _host_image.format();
					new_format.type = retarget_format.type;
					_host_image.setFormat(new_format, _host_image.rowMajor());
					
					if (retarget_format.channels != new_format.channels)
					{
						new_format.channels = retarget_format.channels;
						_host_image.reFormat(new_format);
					}
				}
				_original_format = _host_image.format();
			}

			if (!_host_image.empty())
			{
				if (_desired_format.vk_format == VK_FORMAT_UNDEFINED || _desired_format.vk_format == VK_FORMAT_MAX_ENUM)
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
			VkExtent3D extent = VkExtent3D{ .width = static_cast<uint32_t>(_host_image.width()), .height = static_cast<uint32_t>(_host_image.height()), .depth = 1 };
			if (_desired_layers > 1)
			{
				// Assume layers are store as rows of a single column
				extent.height = extent.height / _desired_layers;
			}
			uint32_t mips = 1;
			if (_desired_mips == MipsOptions::Auto)
			{
				mips = Image::ALL_MIPS;
			}
			_image = std::make_shared<Image>(Image::CI{
				.app = application(),
				.name = name(),
				.type = VK_IMAGE_TYPE_2D,
				.format = _image_format.vk_format,
				.extent = extent,
				.mips = mips,
				.layers = _desired_layers,
				.usage = _image_usages,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});

			// TODO better (good enough for now)
			// Consider: what is an image2D is 1D_ARRAY? Deduce from the file, one layer can be array if we want to, ...
			VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM; // same as auto
			if (_desired_layers > 1)
			{
				view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			}

			_all_mips_view = std::make_shared<ImageView>(ImageView::CI{
				.app = application(),
				.name = name() + "_all_mip"s,
				.image = _image,
				.type = view_type,
			});

			if (mips > 1)
			{
				_top_mip_view = std::make_shared<ImageView>(ImageView::CI{
					.app = application(),
					.name = name() + "_top_mip"s,
					.image = _image,
					.type = view_type,
					.range = VkImageSubresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = _desired_layers,
					},
				});
			}
			else
			{
				_top_mip_view = _all_mips_view;
			}

			_should_upload = true;
		}
	}

	void TextureFromFile::launchLoadTask()
	{
		_load_image_task = std::make_shared<AsynchTask>(AsynchTask::CI{
			.name = name() + ".loadHostImage()",
			.verbosity = AsynchTask::Verbosity::High,
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

	void TextureFromFile::reload()
	{
		if (!_is_synch)
		{
			if (_load_image_task)
			{
				_load_image_task->cancel();
				_load_image_task->waitIFN();
			}
		}
		_image.reset();
		_all_mips_view.reset();
		_top_mip_view.reset();
		_view.reset();
		callResourceUpdateCallbacks();

		if (_is_synch)
		{
			loadHostImage();
			createDeviceImage();
			_view = _all_mips_view;
		}
		else
		{
			launchLoadTask();
		}
	}

	TextureFromFile::TextureFromFile(CreateInfo const& ci):
		Texture(Texture::CI{
			.app = ci.app,
			.name = ci.name,
		}),
		_path(ci.path),
		_is_synch(ci.synch),
		_desired_mips(ci.mips),
		_desired_layers(ci.layers)
	{
		_desired_format = DetailedVkFormat::Find(ci.desired_format);

		reload();
	}

	TextureFromFile::~TextureFromFile()
	{
		if (_load_image_task)
		{
			_load_image_task->cancel();
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
					.data = _host_image.rawData(),
					.size = _host_image.byteSize(),
					.copy_data = false,
					.dst = _top_mip_view->instance(),
				};
				ctx.resourcesToUpload() += std::move(up);
				_upload_done = true;
			}
			else
			{
				if (_load_image_task && _load_image_task->StatusIsFinish(_load_image_task->getStatus()))
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
						// TODO manage this case properly
						_should_upload = false;
						_view = nullptr;
						_image = nullptr;
						_top_mip_view = nullptr;
						_all_mips_view = nullptr;
						callResourceUpdateCallbacks();
					}
					_load_image_task = nullptr;	
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

	void TextureFromFile::declareGUI(GuiContext& ctx)
	{
		ImGui::PushID(this);
		static thread_local std::string txt_buffer;
		txt_buffer = _path.string();
		ImGui::InputText("Path", &txt_buffer, ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine();
		auto file_dialog = ctx.getCommonFileDialog();
		bool can_open = file_dialog->canOpen();
		ImGui::BeginDisabled(!can_open);
		if (ImGui::Button("..."))
		{
			FileDialog::OpenInfo info{};
			if (!_path.empty())
			{
				auto resolved = application()->fileSystem()->resolve(_path);
				if (resolved.result == that::Result::Success)
				{
					resolved = application()->fileSystem()->cannonize(resolved.value);
				}
				if (resolved.result == that::Result::Success)
				{
					info.default_location = resolved.value;
				}
			}
			std::array filters = {
				SDL_DialogFileFilter{
					.name = "Any image file",
					.pattern = "*",
				},
				SDL_DialogFileFilter{
					.name = "PNG image",
					.pattern = "png",
				},
				SDL_DialogFileFilter{
					.name = "JPEG image",
					.pattern = "jpg;jpeg",
				},
				SDL_DialogFileFilter{
					.name = "TGA image",
					.pattern = "tga",
				},
				SDL_DialogFileFilter{
					.name = "HDR image",
					.pattern = "hdr",
				},
			};
			info.filters = filters;
			info.allow_multiple = false;
			info.parent_window = ctx.getCurrentWindow();
			file_dialog->open(this, info);
		}
		ImGui::EndDisabled();

		bool _should_reload = false;
		if (file_dialog->owner() == this && file_dialog->completed())
		{
			if (!file_dialog->getResults().empty())
			{
				that::FileSystem::Path new_path = file_dialog->getResults().front();
				_path = new_path;
				_should_reload = true;
				_is_ready = false;
			}
			file_dialog->close();
		}
		if (_should_reload)
		{
			reload();
		}

		{
			ImVec4 color = ImVec4(1, 1, 1, 1);
			const char* status = "Unknown!";
			if (isReady())
			{
				if ((_desired_mips == MipsOptions::None) || (_view == _all_mips_view))
				{
					status = "Ready!";
				}
				else
				{
					status = "Ready (waiting on MIPs)!";
				}
			}
			else
			{
				if (_host_image.empty())
				{
					if (_path.empty())
					{
						status = "Empty!";
						color = ctx.style().warning_yellow;
					}
					else
					{
						status = "Failed to load image!";
						color = ctx.style().invalid_red;

					}
				}
				else
				{
					status = "Loading...";
					color = ctx.style().warning_yellow;
				}
			}
			ImGui::TextColored(color, "Status: %s", status);
		}

		{
			VkFormat format = VK_FORMAT_UNDEFINED;
			VkExtent3D extent = makeUniformExtent3D(0);
			uint layers = 0;
			uint mips = 0;
			if (isReady())
			{
				auto& instance = _view->instance();
				format = instance->createInfo().format;
				extent = instance->image()->createInfo().extent;
				layers = instance->createInfo().subresourceRange.layerCount;
				mips = instance->createInfo().subresourceRange.levelCount;
			}
			const char * format_str = string_VkFormat(format);
			ImGui::Text("Format: %s", format_str);
			ImGui::InputInt3("Resolution", (int*)&extent.width, ImGuiInputTextFlags_ReadOnly);
			ImGui::InputInt("Mips", (int*) &mips, 0, 0, ImGuiInputTextFlags_ReadOnly);
			ImGui::InputInt("Layers", (int*)&layers, 0, 0, ImGuiInputTextFlags_ReadOnly);
		}
		ImGui::PopID();
	}





	TextureFileCache::TextureFileCache(CreateInfo const& ci):
		VkObject(ci.app, ci.name)
	{}


	std::shared_ptr<TextureFromFile> TextureFileCache::getTexture(std::filesystem::path const& path, VkFormat desired_format)
	{
		std::unique_lock lock(_mutex);
		std::shared_ptr<TextureFromFile> res;
		if (!_cache.contains(path))
		{
			res = std::make_shared<TextureFromFile>(TextureFromFile::CI{
				.app = application(),
				.name = path.string(),
				.path = path,
				.desired_format = desired_format,
			});
			_cache[path] = res;
		}
		else
		{
			res = _cache.at(path);
		}
		return res;
	}

	void TextureFileCache::updateResources(UpdateContext & ctx)
	{
		std::unique_lock lock(_mutex);

		auto it = _cache.begin();

		while (it != _cache.end())
		{
			std::shared_ptr<TextureFromFile> & texture = it->second;

			if (texture.use_count() <= 1)
			{
				it = _cache.erase(it);
			}
			else
			{
				++it;
			}
		}

	}
}