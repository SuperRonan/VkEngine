#include "VkWindow.hpp"
#include "imgui.h"
#include <algorithm>

namespace vkl
{
	void VkWindow::frameBufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		VkWindow* vk_window = reinterpret_cast<VkWindow*>(glfwGetWindowUserPointer(window));
		vk_window->_glfw_resized = true;
	}

	void VkWindow::initGLFW(std::string const& name, int resizeable)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, resizeable);

		DetailedMonitor & monitor = _monitors[_selected_monitor_index];
		glfwWindowHint(GLFW_RED_BITS, monitor.default_vidmode.redBits);
		glfwWindowHint(GLFW_GREEN_BITS, monitor.default_vidmode.greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, monitor.default_vidmode.blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, monitor.default_vidmode.refreshRate);
		glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

		_window = glfwCreateWindow(_width, _height, name.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(_window, this); // Map _window to this
		if (resizeable)
			glfwSetFramebufferSizeCallback(_window, frameBufferResizeCallback);

		saveWindowedAttributes();

		_surface = std::make_shared<Surface>(Surface::CI{
			.app = application(),
			.name = this->name() + ".Surface",
			.window = _window,
		});
		_surface->queryDetails();

		setupGuiObjects();
	}

	void VkWindow::setupGuiObjects()
	{
		const std::vector<Mode> available_window_modes = {
			Mode::Windowed,
			Mode::WindowedFullscreen,
			//Mode::Fullscreen,
			//Mode::ExclusiveFullscreen,
		};

		_gui_window_mode = ImGuiListSelection::CI{
			.name = "Window Mode##"s + name(),
			.mode = ImGuiListSelection::Mode::Combo,
			.options = {
				ImGuiListSelection::Option{
					.name = "Windowed"s,
					.desc = ""s,
				},
				ImGuiListSelection::Option{
					.name = "Windowed Full Screen"s,
					.desc = ""s,
				},
				//ImGuiListSelection::Option{
				//	.name = "Full Screen"s,
				//	.desc = ""s,
				//},
				//ImGuiListSelection::Option{
				//	.name = "Exclusive Full Screen"s,
				//	.desc = ""s,
				//},
			},
		};

		const Surface::SwapchainSupportDetails & sd = _surface->getDetails();
		std::vector<std::string> formats(sd.formats.size());
		std::transform(sd.formats.begin(), sd.formats.end(), formats.begin(), [](VkSurfaceFormatKHR f)
		{
			return getVkFormatName(f.format) + ", "s + getVkColorSpaceKHRName(f.colorSpace);
		});
		const size_t format_index = 0;
		_target_format.setValue(sd.formats[format_index]);
		_gui_formats = ImGuiListSelection::CI{
			.name = "Format##"s + name(),
			.mode = ImGuiListSelection::Mode::Combo,
			.labels = formats,
			.default_index = format_index,
		};
		
		std::vector<ImGuiListSelection::Option> present_modes(sd.present_modes.size());
		for (size_t i = 0; i < present_modes.size(); ++i)
		{
			const VkPresentModeKHR vkp = sd.present_modes[i];
			present_modes[i].name = getVkPresentModeKHRName(vkp);
			switch (vkp)
			{
				case VK_PRESENT_MODE_IMMEDIATE_KHR:
					present_modes[i].desc = "Fastest, Possible Frame Skip"s;
				break;
				case VK_PRESENT_MODE_MAILBOX_KHR:
					present_modes[i].desc = "Fast, No Tearing, Possible Frame Skip"s;
				break;
				case VK_PRESENT_MODE_FIFO_KHR:
					present_modes[i].desc = "VSync, No Tearing, No Frame Skip"s;
				break;
				case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
					present_modes[i].desc = "VSync, No Tearing, Possible Frame Skip"s;
				break;
			}
		}
		const size_t present_mode_index = std::find(sd.present_modes.begin(), sd.present_modes.end(), _target_present_mode.value()) - sd.present_modes.begin();
		
		_gui_present_modes = ImGuiListSelection::CI{
			.name = "Present Mode##"s + name(),
			.mode = ImGuiListSelection::Mode::Combo,
			.options = present_modes,
			.default_index = present_mode_index,
		};
	}


	void VkWindow::createSwapchain()
	{
		_surface->queryDetails();
		_swapchain = std::make_shared<Swapchain>(Swapchain::CI{
			.app = application(),
			.name = name() + ".swapchain",
			.surface = _surface,
			.min_image_count = std::max(_surface->getDetails().capabilities.minImageCount + 1, _surface->getDetails().capabilities.maxImageCount),
			.target_format = _target_format,
			.extent = extract(_dynamic_extent),
			.image_usages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.queues = std::vector<uint32_t>(_queues_families_indices.cbegin(), _queues_families_indices.cend()),
			.target_present_mode = _target_present_mode,
		});
	}

	void VkWindow::DetailedMonitor::query()
	{
		name = glfwGetMonitorName(handle);
		glfwGetMonitorWorkarea(handle, &position.x, &position.y, &pixel_size.x, &pixel_size.y);
		glfwGetMonitorPhysicalSize(handle, &physical_size.x, &physical_size.y);
		glfwGetMonitorContentScale(handle, &scale.x, &scale.y);

		default_vidmode = *glfwGetVideoMode(handle);
		current_vidmode = default_vidmode;

		int n_modes;
		const GLFWvidmode * modes = glfwGetVideoModes(handle, &n_modes);
		available_video_modes.resize(n_modes);
		std::copy_n(modes, n_modes, available_video_modes.data());
		video_mode = glfwGetVideoMode(handle);
	}

	void VkWindow::DetailedMonitor::setGamma(float gamma)
	{
		this->gamma = gamma;
		glfwSetGamma(handle, gamma);
	}

	void VkWindow::init(CreateInfo const& ci)
	{
		_queues_families_indices = ci.queue_families_indices;
		_desired_resolution[0] = ci.w;
		_desired_resolution[1] = ci.h;
		_width = ci.w;
		_height = ci.h;
		_desired_window_mode = ci.mode;

		{
			int monitor_count;
			GLFWmonitor ** monitors = glfwGetMonitors(&monitor_count);
			_monitors.resize(monitor_count);
			std::vector<std::string> monitor_names(monitor_count);
			for (size_t i = 0; i < _monitors.size(); ++i)
			{
				_monitors[i].handle = monitors[i];
				_monitors[i].query();
			}
		}

		_dynamic_extent = [&]() {
			return VkExtent3D{
				.width = _width,
				.height = _height,
				.depth = 1,
			};
		};
		

		_target_present_mode = ci.target_present_mode;

		initGLFW(ci.name, ci.resizeable);
		initSwapchain();
		
	}

	void VkWindow::saveWindowedAttributes()
	{
		_latest_windowed_width = _width;
		_latest_windowed_height = _height;
		glfwGetWindowPos(_window, &_window_pos_x, &_window_pos_y);
	}

	void VkWindow::updateWindowIFP()
	{
		if (_desired_window_mode != _window_mode)
		{
			vkDeviceWaitIdle(_app->device());
			if (_desired_window_mode == Mode::Windowed)
			{
				glfwSetWindowMonitor(_window, nullptr, _window_pos_x, _window_pos_y, _latest_windowed_width, _latest_windowed_height, 0);
			}
			else
			{
				saveWindowedAttributes();

				DetailedMonitor& monitor = _monitors[_selected_monitor_index];

				if (_desired_window_mode == Mode::WindowedFullscreen)
				{
					_width = monitor.default_vidmode.width;
					_height = monitor.default_vidmode.height;
					glfwSetWindowMonitor(_window, nullptr, 0, 0, monitor.default_vidmode.width, monitor.default_vidmode.height, monitor.default_vidmode.refreshRate);
					glfwSetWindowSize(_window, monitor.default_vidmode.width, monitor.default_vidmode.height);
				}
			}
			_window_mode = _desired_window_mode;
			_gui_resized = false;
			_glfw_resized = false;
		}
		else
		{
			if (_glfw_resized)
			{
				int width = 0, height = 0;
				glfwGetFramebufferSize(_window, &width, &height);
				while (width == 0 || height == 0)
				{
					glfwWaitEvents();
					glfwGetFramebufferSize(_window, &width, &height);
				}

				// I don't think it is necessary to do it, but for now we have some validation errors if we don't
				vkDeviceWaitIdle(_app->device());

				setSize(width, height);

				_width = width;
				_height = height;
				_glfw_resized = false;
			}
			else if (_gui_resized)
			{
				vkDeviceWaitIdle(_app->device());
				_width = static_cast<uint32_t>(_desired_resolution[0]);
				_height = static_cast<uint32_t>(_desired_resolution[1]);
				glfwSetWindowSize(_window, _desired_resolution[0], _desired_resolution[1]);
				_glfw_resized = false;
				_gui_resized = false;
			}
		}
	}

	void VkWindow::setSize(uint32_t w, uint32_t h)
	{
		_desired_resolution[0] = w;
		_desired_resolution[1] = h;
	}

	void VkWindow::initSwapchain()
	{
		createSwapchain();
	}

	void VkWindow::cleanupSwapchain()
	{
		_swapchain = nullptr;
	}

	void VkWindow::cleanup()
	{
		cleanupSwapchain();

		_surface = nullptr;
		
		if (_window)
			glfwDestroyWindow(_window);
		_window = nullptr;
	}

	VkWindow::VkWindow(CreateInfo const& ci) :
		VkObject(ci.app, ci.name)
	{
		init(ci);
	}

	VkWindow::~VkWindow()
	{
		cleanup();
	}

	bool VkWindow::shouldClose()const
	{
		return glfwWindowShouldClose(_window);
	}

	void VkWindow::pollEvents()
	{
		glfwPollEvents();
	}

	std::shared_ptr<Image> VkWindow::image(uint32_t index)
	{
		return _swapchain->instance()->images()[index];
	}

	std::shared_ptr<ImageView> VkWindow::view(uint32_t index)
	{
		return _swapchain->instance()->views()[index];
	}


	DynamicValue<VkFormat> VkWindow::format()const
	{
		return [this](){return _target_format.value().format;};
	}

	size_t VkWindow::swapchainSize()const
	{
		return _swapchain->instance()->images().size();
	}

	VkWindow::AquireResult::AquireResult() :
		success(VK_FALSE)
		//semaphore(VK_NULL_HANDLE)
	{}

	VkWindow::AquireResult::AquireResult(uint32_t swap_index) :
		success(VK_TRUE),
		swap_index(swap_index)
		//semaphore(std::move(semaphore)),
		//fence(std::move(fence))
	{}

	VkWindow::AquireResult VkWindow::aquireNextImage(std::shared_ptr<Semaphore> semaphore_to_signal, std::shared_ptr<Fence> _fence_to_signal)
	{
		uint32_t image_index;
		VkSemaphore sem_to_signal = !!semaphore_to_signal ? (VkSemaphore) * semaphore_to_signal : VK_NULL_HANDLE;
		VkFence fence_to_signal = !!_fence_to_signal ? (VkFence) * _fence_to_signal : VK_NULL_HANDLE;
		const VkResult aquire_res = vkAcquireNextImageKHR(_app->device(), *_swapchain->instance(), UINT64_MAX, sem_to_signal, fence_to_signal, &image_index);
		if (aquire_res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			assert(false);
			return AquireResult();
		}
		else if (aquire_res != VK_SUCCESS && aquire_res != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to aquire next swapchain image!");
		}
		_current_frame_info.index = image_index;

		/*if (_image_in_flight_fence[image_index])
		{
			_image_in_flight_fence[image_index]->wait();
		}
		_image_in_flight_fence[image_index] = _in_flight_fence[_current_frame];
		_image_in_flight_fence[image_index]->reset();*/
		return AquireResult(image_index);
	}

	void VkWindow::present(uint32_t num_semaphores, VkSemaphore* semaphores)
	{
		VkSwapchainKHR swapchain = *_swapchain->instance();
		VkPresentInfoKHR presentation{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = num_semaphores,
			.pWaitSemaphores = semaphores,
			.swapchainCount = 1,
			.pSwapchains = &swapchain,
			.pImageIndices = &_current_frame_info.index,
			.pResults = nullptr,
		};
		VkResult present_res = vkQueuePresentKHR(_app->queues().present, &presentation);
		if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR)
		{
			_surface->queryDetails();
			//_swapchain->updateResources();
		}
		else if (present_res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present a swapchain image.");
		}

		{
			const decltype(_present_time_point) now = std::chrono::system_clock::now();
			const auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _present_time_point);
			const std::chrono::milliseconds period = 1000ms;
			if (dt_ms > period)
			{
				const int fps = (1000 * (_current_frame - _present_frame)) / float(dt_ms.count());

				std::string name_to_set = name() + " [" + std::to_string(fps) + " fps]";
				glfwSetWindowTitle(_window, name_to_set.c_str());
				_present_time_point = now;
				_present_frame = _current_frame;

			}
		}

		_current_frame = (_current_frame + 1);
	}

	bool VkWindow::updateResources(UpdateContext & ctx)
	{
		bool res = false;
		updateWindowIFP();
		res |= _swapchain->updateResources(ctx);
		return res;
	}

	void VkWindow::declareGui(GuiContext & ctx)
	{
		if (ImGui::CollapsingHeader("Window"))
		{
			bool changed = false;

			changed = _gui_window_mode.declare();
			if (changed)
			{
				_desired_window_mode = static_cast<Mode>(_gui_window_mode.index());
			}

			const bool can_resize = _window_mode == Mode::Windowed;
			if (can_resize)
			{
				std::string resolution = "Resolution##"s + name();
				changed = ImGui::SliderInt2(resolution.c_str(), _desired_resolution, 1, 3840);
				if (changed)
				{
					_gui_resized = true;
				}
			}
			else
			{
				ImGui::Text("Resolution: %d x %d", _width, _height);
			}

			const Surface::SwapchainSupportDetails& sd = _surface->getDetails();
			changed = _gui_present_modes.declare();
			if (changed)
			{
				_target_present_mode.setValue(sd.present_modes[_gui_present_modes.index()]);
			}

			changed = _gui_formats.declare();
			if (changed)
			{
				_target_format.setValue(sd.formats[_gui_formats.index()]);
			}
		}
	}
}