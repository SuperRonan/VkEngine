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
		VK_CHECK(glfwCreateWindowSurface(_app->instance(), _window, nullptr, &_surface), "Failed to create a surface.");
	}

	VkWindow::SwapchainSupportDetails VkWindow::querySwapChainSupport(VkPhysicalDevice device)
	{
		VkBool32 present_support;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, _app->getQueueFamilyIndices().present_family.value(), _surface, &present_support);
		if (present_support == 0)
		{
			throw std::runtime_error("Physical device cannot present!");
		}

		SwapchainSupportDetails res;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &res.capabilities);

		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, nullptr);
		res.formats.resize(format_count);
		if (format_count != 0)
		{
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, res.formats.data());
		}

		uint32_t present_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_count, nullptr);
		res.present_modes.resize(present_count);
		if (present_count != 0)
		{
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_count, res.present_modes.data());
		}

		return res;
	}

	VkSurfaceFormatKHR VkWindow::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		VkSurfaceFormatKHR target_format{};
		target_format.format = VK_FORMAT_B8G8R8A8_SRGB;
		target_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

		if (std::find_if(formats.cbegin(), formats.cend(), [&target_format](VkSurfaceFormatKHR const& f) {return f.format == target_format.format && f.colorSpace == target_format.colorSpace; }) != formats.cend())
		{
			return target_format;
		}

		// TODO Look for the closest format
		return formats.front();
	}

	VkPresentModeKHR VkWindow::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes, VkPresentModeKHR target)
	{
		VkPresentModeKHR default_mode = VK_PRESENT_MODE_FIFO_KHR;
		VkPresentModeKHR target_mode = target;

		if (std::find(modes.cbegin(), modes.cend(), target_mode) != modes.cend())
		{
			return target_mode;
		}

		return default_mode;
	}

	VkExtent2D VkWindow::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(_window, &width, &height);
			VkExtent2D actual_extent = { (uint32_t)width, (uint32_t)height };
			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			return actual_extent;
		}
	}

	void VkWindow::createSwapchain(VkPresentModeKHR target)
	{
		const SwapchainSupportDetails swap_support = querySwapChainSupport(_app->physicalDevice());
		const VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_support.formats);
		const VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_support.present_modes, target);
		const VkExtent2D extent = chooseSwapExtent(swap_support.capabilities);
		uint32_t swap_size = swap_support.capabilities.minImageCount + 1;
		if (swap_support.capabilities.maxImageCount > 0 && swap_size > swap_support.capabilities.maxImageCount)
		{
			swap_size = swap_support.capabilities.maxImageCount;
		}

		std::vector<uint32_t> queue_family_indices(_queues_families_indices.cbegin(), _queues_families_indices.cend());
		VkSharingMode sharing_mode = queue_family_indices.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		uint32_t queue_count = queue_family_indices.size() == 1 ? 0 : queue_family_indices.size();
		uint32_t* queues_ptr = queue_family_indices.size() == 1 ? nullptr : queue_family_indices.data();

		VkSwapchainCreateInfoKHR swap_ci{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.surface = _surface,
			.minImageCount = swap_size,
			.imageFormat = surface_format.format,
			.imageColorSpace = surface_format.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.imageSharingMode = sharing_mode,
			.queueFamilyIndexCount = queue_count,
			.pQueueFamilyIndices = queues_ptr,
			.preTransform = swap_support.capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = present_mode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		VK_CHECK(vkCreateSwapchainKHR(_app->device(), &swap_ci, nullptr, &_swapchain), "Failed to create a swapchain.");

		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT object_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
				.objectHandle = (uint64_t)_swapchain,
				.pObjectName = name().data(),
			};
			_app->nameObject(object_name);
		}

		_swapchain_image_format = surface_format.format;
		_swapchain_extent = extent;
		uint32_t image_count;
		vkGetSwapchainImagesKHR(_app->device(), _swapchain, &image_count, nullptr);
		_swapchain_images.resize(image_count);
		std::vector<VkImage> tmp_images(image_count);
		vkGetSwapchainImagesKHR(_app->device(), _swapchain, &image_count, tmp_images.data());
		for (uint32_t i = 0; i < image_count; ++i)
		{
			Image::AssociateInfo ai{
				.app = _app,
				.name = name() + std::string(" swapchain image #") + std::to_string(i),
				.image = tmp_images[i],
				.type = VK_IMAGE_TYPE_2D,
				.format = format(),
				.extent = VkExtent3D{.width = extent.width, .height = extent.height, .depth = 1},
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				.queues = {},
			};
			_swapchain_images[i] = std::make_shared<Image>(_app);
			_swapchain_images[i]->associateImage(ai);
		}
	}

	void VkWindow::createSwapchainViews()
	{
		_swapchain_views.resize(_swapchain_images.size());
		for (size_t i = 0; i < _swapchain_views.size(); ++i)
		{
			ImageView::CI ci{
				.name = name() + std::string(" swapchain view #") + std::to_string(i),
				.image = _swapchain_images[i],
				.create_on_construct = true,
			};
			_swapchain_views[i] = std::make_shared<ImageView>(ci);
			//_swapchain_views[i]->createView();
			//_swapchain_views[i] = createImageView(_app->device(), *_swapchain_images[i], _swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
	}

	void VkWindow::createSynchObjects()
	{
		//_image_in_flight_fence.resize(_swapchain_images.size(), nullptr);
		//for (size_t i = 0; i < _swapchain_images.size(); ++i)
		//{
		//	_image_in_flight_fence[i] = std::make_shared<Fence>(_app, true);
		//}
	}

	void VkWindow::init(CreateInfo const& ci)
	{
		_queues_families_indices = ci.queue_families_indices;
		_width = ci.w;
		_height = ci.h;

		_target_present_mode = ci.target_present_mode;

		initGLFW(ci.name, ci.resizeable);
		initSwapchain();
		createSynchObjects();
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
		createSwapchain(_target_present_mode);
		createSwapchainViews();
	}

	void VkWindow::cleanupSwapchain()
	{
		_swapchain_views.clear();
		_swapchain_views.shrink_to_fit();
		_swapchain_images.clear();
		_swapchain_images.shrink_to_fit();
		vkDestroySwapchainKHR(_app->device(), _swapchain, nullptr);
	}

	void VkWindow::cleanup()
	{
		cleanupSwapchain();

		vkDestroySurfaceKHR(_app->instance(), _surface, nullptr);

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