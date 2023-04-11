#pragma once

#include "VkApplication.hpp"

namespace vkl
{
	class Surface : public VkObject
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			GLFWwindow* window = nullptr;
		};
		using CI = CreateInfo;

		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities = {};
			std::vector<VkSurfaceFormatKHR> formats = {};
			std::vector<VkPresentModeKHR> present_modes = {};
		};

	protected:

		GLFWwindow* _window = nullptr;
		VkSurfaceKHR _surface = VK_NULL_HANDLE;

		SwapchainSupportDetails _details;

		void create();

		void destroy();

	public:

		constexpr Surface() noexcept = default;

		Surface(CreateInfo const& ci);

		Surface(Surface const&) = delete;

		Surface& operator=(Surface const&) = delete;

		constexpr Surface(Surface&& other) noexcept :
			VkObject(std::move(other)),
			_window(other._window),
			_surface(other._surface)
		{
			other._window = nullptr;
			other._surface = VK_NULL_HANDLE;
		}

		constexpr Surface& operator=(Surface&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_window, other._window);
			std::swap(_surface, other._surface);
			return *this;
		}

		virtual ~Surface();

		void queryDetails();

		constexpr SwapchainSupportDetails const& getDetails()const
		{
			return _details;
		}

		constexpr VkSurfaceKHR surface() const
		{
			return _surface;
		}

		constexpr VkSurfaceKHR handle() const
		{
			return surface();
		}

		constexpr operator VkSurfaceKHR()const
		{
			return surface();
		}

	};
}