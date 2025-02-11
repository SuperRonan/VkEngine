#include <vkl/VkObjects/VkWindow.hpp>

#include <vkl/VkObjects/DetailedVkFormat.hpp>

#include "imgui.h"
#include <algorithm>
#include <format>

#include <SDL3/SDL_video.h>

namespace vkl
{

#define SDL_WINDOW_FULLSCREEN_DESKTOP (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS)

	void VkWindow::frameBufferResizeCallback(SDL_Window* window, int width, int height)
	{
		NOT_YET_IMPLEMENTED;
		//
		//VkWindow* vk_window = reinterpret_cast<VkWindow*>(SDL_GetWindowData(window, nullptr));
		//vk_window->_sdl_resized = true;
	}


	void VkWindow::preventFlickerWhenFullscreen()
	{
#if _WIN32
		// Prevent flickering when changing focus in fullscreen
		// Thanks to http://forum.lwjgl.org/index.php?topic=4785.0
		// SDL3 appears to handle this natively
		//SDL_SysWMinfo info;
		//SDL_VERSION(&info.version);
		//SDL_GetWindowWMInfo(_window, &info);
		//HWND hwnd = info.info.win.window;
		//LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
		//style &= ~WS_POPUP;
		//SetWindowLongPtr(hwnd, GWL_STYLE, style);
#endif
	}

	void VkWindow::initSDL()
	{
		SDL_WindowFlags flags = 0;
		flags |= SDL_WINDOW_VULKAN;
		if (_resizeable)
		{
			flags |= SDL_WINDOW_RESIZABLE;
		}
		switch (_window_mode)
		{
		case Mode::Fullscreen:
			flags |= SDL_WINDOW_FULLSCREEN;
		break;
		case Mode::WindowedFullscreen:
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		break;
		}
		_window = SDL_CreateWindow(name().c_str(), _width, _height, flags);

		if (_window_mode != Mode::Windowed)
		{
			preventFlickerWhenFullscreen();
		}

		saveWindowedAttributes();

		_surface = std::make_shared<Surface>(Surface::CI{
			.app = application(),
			.name = this->name() + ".Surface",
			.window = _window,
		});
		_surface->queryDetails();
	}

	void VkWindow::setupGuiObjects()
	{
		
		// Window Mode
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
		
		// Format
		if (!_extern_target_format.hasValue())
		{
			MyVector<std::string> formats(sd.formats.size());
			std::transform(sd.formats.begin(), sd.formats.end(), formats.begin(), [](VkSurfaceFormatKHR f)
			{
				return getVkFormatName(f.format) + ", "s + getVkColorSpaceKHRName(f.colorSpace);
			});
			const size_t format_index = 0;
			_target_format = sd.formats[format_index];
			_gui_formats = ImGuiListSelection::CI{
				.name = "Format##"s + name(),
				.mode = ImGuiListSelection::Mode::Combo,
				.labels = formats,
				.default_index = format_index,
			};
		}
		
	
		
		// Present Mode
		if (!_extern_present_mode.hasValue())
		{
			MyVector<ImGuiListSelection::Option> present_modes(sd.present_modes.size());
			for (size_t i = 0; i < present_modes.size(); ++i)
			{
				const VkPresentModeKHR vkp = sd.present_modes[i];
				present_modes[i].name = getVkPresentModeKHRName(vkp);
				switch (vkp)
				{
					case VK_PRESENT_MODE_IMMEDIATE_KHR:
						present_modes[i].desc = "Fastest, Possible Tearing and Frame Skip"s;
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
			const size_t present_mode_index = std::find(sd.present_modes.begin(), sd.present_modes.end(), _target_present_mode) - sd.present_modes.begin();
		
			_gui_present_modes = ImGuiListSelection::CI{
				.name = "Present Mode##"s + name(),
				.mode = ImGuiListSelection::Mode::Combo,
				.options = present_modes,
				.default_index = present_mode_index,
			};
		}
	}

	void VkWindow::deduceColorCorrection()
	{
		if (_swapchain && _swapchain->instance())
		{
			const SwapchainInstance & si = *_swapchain->instance();
			const VkFormat format = si.createInfo().imageFormat;
			const DetailedVkFormat detailed_format = DetailedVkFormat::Find(format);
			const VkColorSpaceKHR color_space = si.createInfo().imageColorSpace;

			ColorCorrectionMode & _mode = _color_correction.mode;
			float & _gamma = _color_correction.params.gamma;
			float & _exposure = _color_correction.params.exposure;

			_gamma = 1.0f;
			_exposure = 1.0f;

			switch (color_space)
			{
			case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:	{
				if (detailed_format.color.type == DetailedVkFormat::Type::SRGB)
				{
					_mode = ColorCorrectionMode::None;
					_gamma = 1.0f;
				}
				else
				{
					_mode = ColorCorrectionMode::sRGB;
					_gamma = 1.0 / 2.4;
				}
			} break;
			case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:	{
				_mode = ColorCorrectionMode::DisplayP3;
			} break;
			case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:	{
				_mode = ColorCorrectionMode::None;
				if (detailed_format.color.type == DetailedVkFormat::Type::SFLOAT || detailed_format.color.type == DetailedVkFormat::Type::UFLOAT)
				{
					_exposure *= 3;
				}
			} break;
			case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:	{
				_mode = ColorCorrectionMode::PassThrough; // TODO check
			} break;
			case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:	{
				_mode = ColorCorrectionMode::DCI_P3;
			} break;
			case VK_COLOR_SPACE_BT709_LINEAR_EXT:	{
				_mode = ColorCorrectionMode::PassThrough; // TODO check
			} break;
			case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:	{
				_mode = ColorCorrectionMode::ITU;
				_gamma = 1.0 / 2.2;
			} break;
			case VK_COLOR_SPACE_BT2020_LINEAR_EXT:	{
				_mode = ColorCorrectionMode::PassThrough; // TODO check
			} break;
			case VK_COLOR_SPACE_HDR10_ST2084_EXT:	{
				_mode = ColorCorrectionMode::PerceptualQuantization;
				_exposure *= 128 * 2;
			} break;
			case VK_COLOR_SPACE_DOLBYVISION_EXT:	{
				_mode = ColorCorrectionMode::HybridLogGamma;
			} break;
			case VK_COLOR_SPACE_HDR10_HLG_EXT:	{
				_mode = ColorCorrectionMode::HybridLogGamma;
			} break;
			case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:	{
				_mode = ColorCorrectionMode::PassThrough; // TODO check
			} break;
			case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:	{
				_mode = ColorCorrectionMode::AdobeRGB;
			} break;
			case VK_COLOR_SPACE_PASS_THROUGH_EXT:	{
				_mode = ColorCorrectionMode::PassThrough; // TODO check
			} break;
			case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:	{
				_mode = ColorCorrectionMode::scRGB;
				_gamma = 1.0 / 2.4;
			} break;
			case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:	{
				_mode = ColorCorrectionMode::PassThrough; // TODO check
			} break;
			}
		}
	}

	void VkWindow::createSwapchain()
	{
		_surface->queryDetails();
		_swapchain = std::make_shared<Swapchain>(Swapchain::CI{
			.app = application(),
			.name = name() + ".swapchain",
			.surface = _surface,
			.min_image_count = std::max(_surface->getDetails().capabilities.minImageCount + 1, _surface->getDetails().capabilities.maxImageCount),
			.target_format = &_target_format,
			.extent = extract(_dynamic_extent),
			.image_usages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.queues = std::vector<uint32_t>(_queues_families_indices.cbegin(), _queues_families_indices.cend()),
			.target_present_mode = &_target_present_mode,
		});
	}

	void VkWindow::init(CreateInfo const& ci)
	{
		_queues_families_indices = ci.queue_families_indices;

		_resizeable = ci.resizeable;

		if (ci.dynamic_resolution.hasValue())
		{
			_extern_resolution = ci.dynamic_resolution;
			_desired_resolution = _extern_resolution.value();
			assert(!_resizeable);
		}
		else
		{
			_desired_resolution = ci.resolution;
		}

		_width = _desired_resolution[0];
		_height = _desired_resolution[1];
		
		if(ci.dynamic_present_mode.hasValue())
		{
			_extern_present_mode = ci.dynamic_present_mode;
			_target_present_mode = _extern_present_mode.value();
		}
		else
		{
			_target_present_mode = ci.present_mode;	
		}

		if (ci.dynamic_target_format.hasValue())
		{
			_extern_target_format = ci.dynamic_target_format;
			_target_format = _extern_target_format.value();
		}
		else
		{
			
		}

		_desired_window_mode = ci.mode;

		{
			// TODO
			//int monitor_count;
			//GLFWmonitor ** monitors = glfwGetMonitors(&monitor_count);
			//_monitors.resize(monitor_count);
			//std::vector<std::string> monitor_names(monitor_count);
			//for (size_t i = 0; i < _monitors.size(); ++i)
			//{
			//	_monitors[i].handle = monitors[i];
			//	_monitors[i].query();
			//}
		}

		_dynamic_extent = [&]() {
			return VkExtent3D{
				.width = _width,
				.height = _height,
				.depth = 1,
			};
		};

		initSDL();
		
		setupGuiObjects();

		initSwapchain();
		
	}

	void VkWindow::saveWindowedAttributes()
	{
		_latest_windowed_width = _width;
		_latest_windowed_height = _height;
		//glfwGetWindowPos(_window, &_window_pos_x, &_window_pos_y);
	}

	void VkWindow::updateWindowIFP()
	{
		if (_extern_resolution.hasValue())
		{
			_desired_resolution = _extern_resolution.value();
		}
		if (_extern_present_mode.hasValue())
		{
			_target_present_mode = _extern_present_mode.value();
		}
		if (_extern_target_format.hasValue())
		{
			_target_format = _extern_target_format.value();
		}

		if (_desired_window_mode != _window_mode)
		{
			application()->deviceWaitIdle();
			if (_desired_window_mode == Mode::Windowed)
			{
				SDL_SetWindowFullscreen(_window, 0);
			}
			else
			{
				saveWindowedAttributes();

				uint32_t flags = 0;
				if (_desired_window_mode == Mode::WindowedFullscreen)
				{
					flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
				}
				else if (_desired_window_mode == Mode::Fullscreen)
				{
					flags |= SDL_WINDOW_FULLSCREEN;
				}
				SDL_SetWindowFullscreen(_window, flags);
				
				preventFlickerWhenFullscreen();
				//SDL_SetWindowBordered(_window, SDL_TRUE);
				//SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

				VKL_BREAKPOINT_HANDLE;
			}
			
			_window_mode = _desired_window_mode;
			_gui_resized = false;
			_sdl_resized = false;
		}
		else
		{
			if (_sdl_resized)
			{
				int width = 0, height = 0;
				SDL_GetWindowSize(_window, &width, &height);
				// What if width of height == 0?
				//while (width == 0 || height == 0)
				//	glfwWaitEvents();

				// I don't think it is necessary to do it, but for now we have some validation errors if we don't
				application()->deviceWaitIdle();

				setSize(width, height);

				_width = width;
				_height = height;
				_sdl_resized = false;
			}
			else if (_gui_resized)
			{
				application()->deviceWaitIdle();
				
				_width = static_cast<uint32_t>(_desired_resolution[0]);
				_height = static_cast<uint32_t>(_desired_resolution[1]);
				SDL_SetWindowSize(_window, _desired_resolution[0], _desired_resolution[1]);
				_sdl_resized = false;
				_gui_resized = false;
			}
		}
	}

	void VkWindow::setSize(uint32_t w, uint32_t h)
	{
		assert(!_extern_resolution.hasValue());
		_desired_resolution = {w, h};
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
		{
			SDL_DestroyWindow(_window);
			_window = nullptr;
		}
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

	bool VkWindow::eventIsRelevent(SDL_Event const& event) const
	{
		return (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) && (SDL_GetWindowFromID(event.window.windowID) == _window);
	}

	bool VkWindow::processEventCheckRelevent(SDL_Event const& event)
	{
		bool res = false;
		if (eventIsRelevent(event))
		{
			res = true;
			processEventAssumeRelevent(event);	
		}
		return res;
	}

	void VkWindow::processEventAssumeRelevent(SDL_Event const& event)
	{
		SDL_WindowEvent const& wevent = event.window;
		switch (event.type)
		{
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			_should_close = true;
			break;
		case SDL_EVENT_WINDOW_RESIZED:
			_desired_resolution = Vector2i(wevent.data1, wevent.data2);
			_sdl_resized = true;
		break;
		}

		//if (false)
		//{
		//	const char * events_name[] = { 
		//		"SDL_WINDOWEVENT_NONE",   
		//		"SDL_WINDOWEVENT_SHOWN",          
		//		"SDL_WINDOWEVENT_HIDDEN",         
		//		"SDL_WINDOWEVENT_EXPOSED",        
		//		"SDL_WINDOWEVENT_MOVED",          
		//		"SDL_WINDOWEVENT_RESIZED",        
		//		"SDL_WINDOWEVENT_SIZE_CHANGED",   
		//		"SDL_WINDOWEVENT_MINIMIZED",      
		//		"SDL_WINDOWEVENT_MAXIMIZED",      
		//		"SDL_WINDOWEVENT_RESTORED",       
		//		"SDL_WINDOWEVENT_ENTER",          
		//		"SDL_WINDOWEVENT_LEAVE",          
		//		"SDL_WINDOWEVENT_FOCUS_GAINED",   
		//		"SDL_WINDOWEVENT_FOCUS_LOST",     
		//		"SDL_WINDOWEVENT_CLOSE",          
		//		"SDL_WINDOWEVENT_TAKE_FOCUS",     
		//		"SDL_WINDOWEVENT_HIT_TEST",       
		//		"SDL_WINDOWEVENT_ICCPROF_CHANGED",
		//		"SDL_WINDOWEVENT_DISPLAY_CHANGED" 
		//	};
		//	std::cout << "Window event: " << events_name[wevent.event] << std::endl;
		//}
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
		return [this](){return _target_format.format;};
	}

	size_t VkWindow::swapchainSize()const
	{
		return _swapchain->instance()->images().size();
	}

	VkWindow::AquireResult::AquireResult() :
		success(VK_FALSE),
		swap_index(0)
	{}

	VkWindow::AquireResult::AquireResult(uint32_t swap_index) :
		success(VK_TRUE),
		swap_index(swap_index)
	{}

	VkWindow::AquireResult VkWindow::aquireNextImage(std::shared_ptr<Semaphore> semaphore_to_signal, std::shared_ptr<Fence> _fence_to_signal)
	{
		uint32_t image_index;
		VkSemaphore sem_to_signal = !!semaphore_to_signal ? (VkSemaphore) * semaphore_to_signal : VK_NULL_HANDLE;
		VkFence fence_to_signal = !!_fence_to_signal ? (VkFence) * _fence_to_signal : VK_NULL_HANDLE;
		const VkResult aquire_res = vkAcquireNextImageKHR(_app->device(), *_swapchain->instance(), UINT64_MAX, sem_to_signal, fence_to_signal, &image_index);
		if (aquire_res == VK_ERROR_OUT_OF_DATE_KHR || aquire_res == VK_SUBOPTIMAL_KHR)
		{
			NOT_YET_IMPLEMENTED;
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

	//void VkWindow::present(uint32_t num_semaphores, VkSemaphore* semaphores, VkFence fence)
	//{
	//	VkSwapchainKHR swapchain = *_swapchain->instance();
	//	VkPresentInfoKHR presentation{
	//		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	//		.pNext = nullptr,
	//		.waitSemaphoreCount = num_semaphores,
	//		.pWaitSemaphores = semaphores,
	//		.swapchainCount = 1,
	//		.pSwapchains = &swapchain,
	//		.pImageIndices = &_current_frame_info.index,
	//		.pResults = nullptr,
	//	};
	//	VkSwapchainPresentFenceInfoEXT present_fence;

	//	if (fence)
	//	{
	//		if (application()->availableFeatures().swapchain_maintenance1_ext.swapchainMaintenance1)
	//		{
	//			present_fence = VkSwapchainPresentFenceInfoEXT{
	//				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
	//				.pNext = nullptr,
	//				.swapchainCount = 1,
	//				.pFences = &fence,
	//			};
	//			presentation.pNext = &present_fence;
	//		}
	//	}

	//	VkResult present_res = vkQueuePresentKHR(_app->queues().present, &presentation);
	//	if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR)
	//	{
	//		_surface->queryDetails();
	//		//_swapchain->updateResources();
	//	}
	//	else if (present_res != VK_SUCCESS)
	//	{
	//		throw std::runtime_error("Failed to present a swapchain image.");
	//	}

	//	{
	//		const decltype(_present_time_point) now = std::chrono::system_clock::now();
	//		const auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _present_time_point);
	//		const std::chrono::milliseconds period = 1000ms;
	//		if (dt_ms > period)
	//		{
	//			const int fps = (1000 * (_current_frame - _present_frame)) / float(dt_ms.count());

	//			std::string name_to_set = name() + " [" + std::to_string(fps) + " fps]";
	//			SDL_SetWindowTitle(_window, name_to_set.c_str());
	//			_present_time_point = now;
	//			_present_frame = _current_frame;

	//		}
	//	}

	//	_current_frame = (_current_frame + 1);
	//}

	void VkWindow::notifyPresentResult(VkResult present_res)
	{
		if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR)
		{
			_surface->queryDetails();
			//_swapchain->updateResources();
		}

		{
			const decltype(_present_time_point) now = std::chrono::system_clock::now();
			const auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _present_time_point);
			const std::chrono::milliseconds period = 1000ms;
			if (dt_ms > period)
			{
				const int fps = (1000 * (_current_frame - _present_frame)) / float(dt_ms.count());

				std::string name_to_set = name() + " [" + std::to_string(fps) + " fps]";
				SDL_SetWindowTitle(_window, name_to_set.c_str());
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

		deduceColorCorrection();

		return res;
	}

	std::string GetSDLDisplayModeAsStringWithFormat(SDL_DisplayMode const& dm)
	{
		const char * fmt_name = SDL_GetPixelFormatName(dm.format);
		std::string res = std::format("{}, {}x{} @ {}Hz", fmt_name, dm.w, dm.h, dm.refresh_rate);
		return res;
	}

	std::string GetSDLDisplayModeAsString(SDL_DisplayMode const& dm)
	{
		std::string res = std::format("{}x{} @ {}Hz", dm.w, dm.h, dm.refresh_rate);
		return res;
	}
	
	std::string GetSDLDisplayModeAsString(const SDL_DisplayMode * dm, bool with_format = false)
	{
		std::string res;
		if (dm)
		{
			if (with_format)
			{
				res = GetSDLDisplayModeAsStringWithFormat(*dm);
			}
			else
			{
				res = GetSDLDisplayModeAsString(*dm);
			}
		}
		else
		{
			res = "No display mode!";
		}
		return res;
	}

	void VkWindow::declareGui(GuiContext & ctx)
	{
		ImGui::PushID(this);
		if (ImGui::CollapsingHeader("Window"))
		{
			bool changed = false;

			changed = _gui_window_mode.declare();
			if (changed)
			{
				_desired_window_mode = static_cast<Mode>(_gui_window_mode.index());
			}

			const bool can_resize = _window_mode == Mode::Windowed && !_extern_resolution.hasValue();
			if (can_resize)
			{
				changed = ImGui::SliderInt2("Resolution: ", &_desired_resolution[0], 1, 3840);
				if (changed)
				{
					_gui_resized = true;
				}
			}
			else
			{
				ImGui::BeginDisabled(true);
				ImGui::DragInt2("Resolution", &_desired_resolution[0], 1, 3840);
				ImGui::EndDisabled();
			}

			const Surface::SwapchainSupportDetails& sd = _surface->getDetails();
			
			if (_extern_present_mode.hasValue())
			{
				if (_swapchain)
				{
					VkPresentModeKHR pm = _swapchain->instance() ? _swapchain->instance()->createInfo().presentMode :  _swapchain->presentMode().value();
					ImGui::Text("Present Mode: %s", getVkPresentModeKHRName(pm));
				}
			}
			else
			{
				changed = _gui_present_modes.declare();
				if (changed)
				{
					_target_present_mode = (sd.present_modes[_gui_present_modes.index()]);
				}
			}

			if (_extern_target_format.hasValue())
			{
				if (_swapchain)
				{
					VkSurfaceFormatKHR sfmt = _swapchain->format().value();
					ImGui::Text("Format: %s", getVkFormatName(sfmt.format));
					ImGui::Text("Color Space: %s", getVkColorSpaceKHRName(sfmt.colorSpace));
				}
			}
			else
			{
				changed = _gui_formats.declare();
				if (changed)
				{
					_target_format = (sd.formats[_gui_formats.index()]);
				}
			}
			float max_brightness = _color_correction.mode == ColorCorrectionMode::PerceptualQuantization ? 1e4 : 10;
			ImGui::SliderFloat("Brightness", &_brightness, 0, max_brightness, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
			
			const bool display_mode_read_only = (_window_mode == Mode::Windowed) || true;
			if (display_mode_read_only)
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::BeginCombo("Display Mode", GetSDLDisplayModeAsString(SDL_GetWindowFullscreenMode(_window)).c_str()))
			{
				SDL_DisplayID display_index = SDL_GetDisplayForWindow(_window);
				int num_display_mode;
				SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display_index, &num_display_mode);
				if (modes)
				{
					for (int i = 0; i < num_display_mode; ++i)
					{
						if (ImGui::Selectable(GetSDLDisplayModeAsString(*modes[i]).c_str(), _desired_display_mode_index == i))
						{
							_desired_display_mode_index = i;
						}
					}
				}
				ImGui::EndCombo();
			}
			if (display_mode_read_only)
			{
				ImGui::EndDisabled();
			}

			SDL_DisplayID display_index = SDL_GetDisplayForWindow(_window);
			int num_displays;
			const SDL_DisplayID* displays = SDL_GetDisplays(&num_displays);
			const bool display_read_only = (_window_mode == Mode::Windowed) || true;
			if (display_read_only)
			{
				ImGui::BeginDisabled();
			}
			changed = false;
			if (ImGui::BeginCombo("Display", SDL_GetDisplayName(display_index)))
			{
				for (int i = 0; i < num_displays; ++i)
				{
					const bool active = display_index == i;
					if (ImGui::Selectable(SDL_GetDisplayName(i), active))
					{
						_desired_monitor_index = i;
					}
				}
				ImGui::EndCombo();
			}
			if (display_read_only)
			{
				ImGui::EndDisabled();
			}

		}
		ImGui::PopID();
	}
}