#include <Core/Rendering/ImageSaver.hpp>

#include <Core/Commands/PrebuiltTransferCommands.hpp>

#include <Core/VkObjects/DetailedVkFormat.hpp>

#include <img/Image.hpp>
#include <img/ImWrite.hpp>

namespace vkl
{
	ImageSaver::ImageSaver(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_src(ci.src),
		_dst_folder(ci.dst_folder),
		_dst_filename(ci.dst_filename)
	{
		
	}

	ImageSaver::~ImageSaver()
	{

	}

	void ImageSaver::updateResources(UpdateContext& ctx)
	{
		
	}

	void ImageSaver::execute(ExecutionRecorder& exec)
	{
		if (_save_image)
		{
			DownloadImage & downloader = application()->getPrebuiltTransferCommands().download_image;

			struct SaveInfo
			{
				std::filesystem::path full_path;
				std::string extension;

				VkExtent3D extent;
				VkFormat format;

				bool emit_delayed_task;
			};
			std::filesystem::path path = _dst_folder / (_dst_filename + std::to_string(_index));
			if (!_format.empty())
			{
				path += _format;
			}
			std::shared_ptr<SaveInfo> ptr_save_info = std::make_shared<SaveInfo>(SaveInfo{
				.full_path = std::move(path),
				.extension = _format,
				.extent = _src->instance()->image()->createInfo().extent,
				.format = _src->instance()->createInfo().format,
				.emit_delayed_task = false,
			});

			DownloadCallback callback = [ptr_save_info](int vk_res_int, std::shared_ptr<PooledBuffer> const& buffer)
			{
				SaveInfo & save_info = *ptr_save_info;
				auto save_image_f = [&]() -> int
				{
					int res = 0;
					DetailedVkFormat detailed_format = DetailedVkFormat::Find(save_info.format);
					that::FormatInfo format = detailed_format.getImgFormatInfo();
					that::img::FormatedImage image(save_info.extent.width, save_info.extent.height, format, true);

					buffer->buffer()->flush();
					void * data = buffer->buffer()->map();
					std::memcpy(image.rawData(), data, image.byteSize());
					buffer->buffer()->unMap();

					that::img::io::WriteInfo write_info{
					};
					that::img::io::write(std::move(image), save_info.full_path, write_info);

					return res;
				};

				VkResult vk_res = static_cast<VkResult>(vk_res_int);
				if (vk_res == VK_SUCCESS)
				{
					if (save_info.emit_delayed_task)
					{
						std::shared_ptr<AsynchTask> task = std::make_shared<AsynchTask>(AsynchTask::CI{
							.name = "Saving Image "s + save_info.full_path.string(),
							.verbosity = 1,
							.priority = TaskPriority::Soon(),
							.lambda = [=]()
							{
								int success = save_image_f();

								AsynchTask::ReturnType result{
									.success = true,
								};
								if (success != 0)
								{
									result.success = false;
									result.can_retry = false;
								}
								return result;
							},
						});
					}
					else
					{
						save_image_f();
					}
				}
			};

			exec(downloader(DownloadImage::DownloadInfo{
				.src = _src,
				.completion_callback = callback,
			}));
			++_index;
			_save_image = false;
		}
	}

	void ImageSaver::declareGUI(GuiContext& ctx)
	{
		ImGui::PushID(this);

		_save_image = ImGui::Button("Save Image");

		ImGui::PopID();
	}
}