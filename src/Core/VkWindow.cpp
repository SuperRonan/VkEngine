#include "VkWindow.hpp"

#include <algorithm>

namespace vkl
{
	void VkWindow::frameBufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		VkWindow* vk_window = reinterpret_cast<VkWindow*>(glfwGetWindowUserPointer(window));
		vk_window->_framebuffer_resized = true;
	}

	void VkWindow::initGLFW(std::string const& name, int resizeable)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, resizeable);
		_window = glfwCreateWindow(_width, _height, name.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(_window, this); // Map _window to this
		if (resizeable)
			glfwSetFramebufferSizeCallback(_window, frameBufferResizeCallback);

		_surface = std::make_shared<Surface>(Surface::CI{
			.app = application(),
			.name = this->name() + ".Surface",
			.window = _window,
		});
	}


	void VkWindow::createSwapchain()
	{
		_swapchain = std::make_shared <Swapchain>(Swapchain::CI{
			.app = application(),
			.name = name() + ".swapchain",
			.surface = _surface,
			.min_image_count = _surface->getDetails().capabilities.minImageCount + 1,
			.extent = VkExtent2D{.width = _width, .height = _height},
			.image_usages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.queues = std::vector<uint32_t>(_queues_families_indices.cbegin(), _queues_families_indices.cend()),
			.target_present_mode = _target_present_mode,
		});
	}

	void VkWindow::init(CreateInfo const& ci)
	{
		_queues_families_indices = ci.queue_families_indices;
		_width = ci.w;
		_height = ci.h;


		_target_present_mode = ci.target_present_mode;

		initGLFW(ci.name, ci.resizeable);
		initSwapchain();
		
		if (ci.resizeable == GLFW_FALSE)
		{
			VkExtent3D ext = VkExtent3D{ .width = _width, .height = _height, .depth = 1 };
			_dynamic_extent = std::make_shared <DynamicValue<VkExtent3D>>(ext);
		}
		else
		{
			_dynamic_extent = std::make_shared<LambdaValue<VkExtent3D>>([&]() {
				return VkExtent3D{
					.width = _width,
					.height = _height,
					.depth = 1,
				};
			});
		}
	}

	void VkWindow::reCreateSwapchain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(_window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwWaitEvents();
			glfwGetFramebufferSize(_window, &width, &height);
		}
		_width = width;
		_height = height;
		vkDeviceWaitIdle(_app->device());

		cleanupSwapchain();
		initSwapchain();
		_framebuffer_resized = false;
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

	bool VkWindow::framebufferResized()
	{
		return _framebuffer_resized;
	}

	std::shared_ptr<Image> VkWindow::image(uint32_t index)
	{
		return _swapchain_images[index];
	}

	std::shared_ptr<ImageView> VkWindow::view(uint32_t index)
	{
		return _swapchain_views[index];
	}

	VkFormat VkWindow::format()const
	{
		return _swapchain_image_format;
	}

	size_t VkWindow::swapchainSize()const
	{
		return _swapchain_images.size();
	}

	VkExtent2D VkWindow::extent()const
	{
		return _swapchain_extent;
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
		const VkResult aquire_res = vkAcquireNextImageKHR(_app->device(), _swapchain, UINT64_MAX, sem_to_signal, fence_to_signal, &image_index);
		if (aquire_res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			reCreateSwapchain();
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
		VkPresentInfoKHR presentation{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = num_semaphores,
			.pWaitSemaphores = semaphores,
			.swapchainCount = 1,
			.pSwapchains = &_swapchain,
			.pImageIndices = &_current_frame_info.index,
			.pResults = nullptr,
		};
		VkResult present_res = vkQueuePresentKHR(_app->queues().present, &presentation);
		if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR)
		{
			reCreateSwapchain();
		}
		else if (present_res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present a swapchain image.");
		}

		{
			const decltype(_present_time_point) now = std::chrono::system_clock::now();
			const auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _present_time_point);
			if (dt_ms > std::chrono::milliseconds(1000))
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
}