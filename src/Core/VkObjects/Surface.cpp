#include "Surface.hpp"
#include <cassert>

namespace vkl
{
	void Surface::create()
	{
		assert(_surface == VK_NULL_HANDLE);
		SDL_bool res =  SDL_Vulkan_CreateSurface(_window, _app->instance(), &_surface);
		assertm(res, "Failed to create a surface.");
		queryDetails();
	}

	void Surface::queryDetails()
	{
		VkPhysicalDevice device = application()->physicalDevice();
		VkBool32 present_support;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, _target_queue_family, _surface, &present_support);
		if (present_support == 0)
		{
			throw std::runtime_error("Physical device cannot present!");
		}

		SwapchainSupportDetails & res = _details;

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
	}

	void Surface::destroy()
	{
		assert(_surface != VK_NULL_HANDLE);
		vkDestroySurfaceKHR(_app->instance(), _surface, nullptr);

		_surface = VK_NULL_HANDLE;
		_window = nullptr;
	}

	Surface::Surface(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_window(ci.window),
		_target_queue_family(ci.target_queue_family)
	{
		create();
	}

	Surface::~Surface()
	{
		if (_surface != VK_NULL_HANDLE)
		{
			destroy();
		}
	}
}