#include <vkl/Rendering/ImageSaver.hpp>

#include <vkl/Commands/PrebuiltTransferCommands.hpp>

#include <vkl/VkObjects/DetailedVkFormat.hpp>

#include <vkl/Execution/ThreadPool.hpp>

#include <that/img/Image.hpp>
#include <that/img/ImWrite.hpp>

#include <imgui/misc/cpp/imgui_stdlib.h>

#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/Panel.hpp>
#include <vkl/GUI/PathWidget.hpp>

#include <format>

namespace vkl
{
	static std::array extensions_list = {
		".png",
		".jpg",
		".hdr",
	};

	enum FormatIndex
	{
		PNG, 
		JPG,
		HDR,
	};

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
		_mutex.lock();
		while (!_pending_tasks.empty())
		{
			std::shared_ptr<AsynchTask> & task = _pending_tasks.front();
			if (task->StatusIsFinish(task->getStatus()))
			{
				_pending_tasks.pop_front();
			}
			else
			{
				break;
			}
		}
		while(_pending_tasks.size() > _pending_capacity)
		{
			std::shared_ptr<AsynchTask>& task = _pending_tasks.front();
			task->waitIFN();
			_pending_tasks.pop_front();
		}
		_mutex.unlock();
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

				std::string vk_image_name;

				DelayedTaskExecutor * thread_pool = nullptr;
			};
			std::string filename = nullptr;
			if (_dst_filename.empty())
			{
				filename = std::format("{}_", application()->getProjectName());
			}
			else
			{
				filename = _dst_filename;
			}
			std::filesystem::path path = _dst_folder / (filename + std::to_string(_index));
			if (!_extension.empty())
			{
				path += _extension;
			}
			std::shared_ptr<SaveInfo> ptr_save_info = std::make_shared<SaveInfo>(SaveInfo{
				.full_path = std::move(path),
				.extension = _extension,
				.extent = _src->instance()->image()->createInfo().extent,
				.format = _src->instance()->createInfo().format,
				.vk_image_name = _src->name(),
				.thread_pool = _save_in_separate_thread ? &application()->threadPool() : nullptr,
			});

			DownloadCallback callback = [ptr_save_info, this](int vk_res_int, std::shared_ptr<PooledBuffer> const& buffer_ref)
			{
				std::shared_ptr<PooledBuffer> buffer = buffer_ref;
				auto save_image_f = [ptr_save_info, this, vk_res_int, buffer]() -> that::Result
				{
					that::Result result = that::Result::Success;
					DetailedVkFormat detailed_format = DetailedVkFormat::Find(ptr_save_info->format);
					that::FormatInfo format = detailed_format.getImgFormatInfo();
					that::img::FormatedImage image(ptr_save_info->extent.width, ptr_save_info->extent.height, format, true);

					buffer->buffer()->flush();
					void * data = buffer->buffer()->map();
					std::memcpy(image.rawData(), data, image.byteSize());
					buffer->buffer()->unMap();
					
					if (this->_strip_alpha && format.channels == 4)
					{
						format.channels = 3;
						image.reFormat(format);
					}

					that::img::io::WriteInfo write_info{
						.quality = _jpg_quality,
						.row_major = true,
						.can_modify_image = true,
						.format = format,
						.image = &image,
						.path = &ptr_save_info->full_path,
						.filesystem = application()->fileSystem(),
					};
					that::Result write_result = that::img::io::Write(write_info);
					if (write_result != that::Result::Success)
					{
						result = write_result;
					}
					return result;
				};

				VkResult vk_res = static_cast<VkResult>(vk_res_int);
				if (vk_res == VK_SUCCESS)
				{
					if (ptr_save_info->thread_pool)
					{
						std::shared_ptr<AsynchTask> task = std::make_shared<AsynchTask>(AsynchTask::CI{
							.name = "Saving Image "s + ptr_save_info->full_path.string(),
							.verbosity = 1,
							.priority = TaskPriority::Soon(),
							.lambda = [=]()
							{
								that::Result res = save_image_f();

								AsynchTask::ReturnType result{
									.success = true,
								};
								if (res != that::Result::Success)
								{
									result.success = false;
									result.can_retry = false;
									result.error_title = "Failed to write image!";
									result.error_message = "Failed to write VkImage ["s + ptr_save_info->vk_image_name + "] to "s + ptr_save_info->full_path.string() + "\n";
									result.error_message += "Error code: "s + that::GetResultStrSafe(res) + std::format(" ({:#x})", static_cast<int>(res));
								}
								return result;
							},
						});
						ptr_save_info->thread_pool->pushTask(task);
						_mutex.lock();
						_pending_tasks.push_back(std::move(task));
						_mutex.unlock();
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
	namespace GUI
	{
		class ImageSaverInspector : public Panel
		{
		protected:

			std::shared_ptr<ImageSaver> _target;

			ImGuiListSelection _extension;

			PathWidget _path;

		public:

			ImageSaverInspector(std::shared_ptr<ImageSaver> const& target) :
				Panel(target->application(), std::format("{}", target->name())),
				_target(target)
			{
				MyVector<ImGuiListSelection::Option> extensions_options(extensions_list.size() + 1);
				for (size_t i = 0; i < extensions_list.size(); ++i)
				{
					extensions_options[i].name = extensions_list[i];
				}
				extensions_options.back().name = "AUTO";
				extensions_options.back().desc = "Deduce from image pixel type";
				_extension = ImGuiListSelection::CI{
					.name = "File type",
					.mode = ImGuiListSelection::Mode::RadioButtons,
					.same_line = true,
					.options = std::move(extensions_options),
					.default_index = extensions_list.size(),
				};
				_path.label = "Directory";
				_path.text_edit_flags = ImGuiInputTextFlags_EnterReturnsTrue;
				_path.mode = FileDialog::Mode::OpenFolder;
				_path.setPath(_target->_dst_folder);
			}

			virtual void declareInline(Context& ctx) override
			{
				_target->_save_image = ImGui::Button("Save Image");

				ImGui::PushID(0);
				if (_path.declareInline(ctx))
				{
					_target->_dst_folder = _path.path;
				}
				ImGui::PopID();

				ImGui::PushID(1);
				ImGui::TextFieldEdit("Filename", &_target->_dst_filename, application()->getProjectName());
				ImGui::PopID();

				ImGui::InputInt("Index", reinterpret_cast<int*>(&_target->_index));

				if (_extension.declare())
				{
					const size_t index = _extension.index();
					if (index < extensions_list.size())
					{
						_target->_extension = extensions_list[index];
					}
					else
					{
						_target->_extension.clear();
					}
				}
				if (_extension.index() == FormatIndex::JPG)
				{
					ImGui::SliderInt("Quality", &_target->_jpg_quality, 0, 100);
				}

				ImGui::Checkbox("Stip alpha channel", &_target->_strip_alpha);

				ImGui::Separator();
				ImGui::Checkbox("Create Folder IFN", &_target->_create_folder_ifn);
				ImGui::SameLine();
				ImGui::Checkbox("Multi-Threaded", &_target->_save_in_separate_thread);
				if (_target->_save_in_separate_thread)
				{
					ImGui::InputInt("Save Queue Capacity", reinterpret_cast<int*>(&_target->_pending_capacity));
					ImGui::BeginDisabled();
					int queue_size = _target->_pending_tasks.size();
					const Vector3f red(1, 0, 0);
					const Vector3f yellow(1, 1, 0);
					const Vector3f green(0, 1, 0);
					const float charge = float(queue_size) / float(_target->_pending_capacity);
					Vector3f color;
					if (charge < 0.5)
					{
						color = Lerp(green, yellow, charge * 2.0f);
					}
					else
					{
						color = Lerp(yellow, red, (charge - 0.5f) * 2.0f);
					}
					ImVec4 gui_color(color.x(), color.y(), color.z(), 1);
					ImGui::PushStyleColor(ImGuiCol_Text | ImGuiCol_SliderGrab, gui_color);
					ImGui::SliderInt("Save Queue", &queue_size, 0, _target->_pending_capacity);
					ImGui::PopStyleColor();
					ImGui::EndDisabled();
					ImGui::Separator();
				}
			}
		};
	}

	std::shared_ptr<GUI::Panel> ImageSaver::makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx)
	{
		assert(shared_this.get() == this);
		return std::make_shared<GUI::ImageSaverInspector>(std::static_pointer_cast<ImageSaver>(shared_this));
	}
}