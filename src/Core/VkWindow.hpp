#pragma once

#include "VkApplication.hpp"

#include <set>
#include <string>
#include <vector>
#include "ImageView.hpp"
#include "Semaphore.hpp"
#include "Fence.hpp"

namespace vkl
{

	class VkWindow : public VkObject
	{
	public:

		struct CreateInfo
		{	
			VkApplication* app;
			std::set<uint32_t> queue_families_indices;
			uint32_t in_flight_size;

			VkPresentModeKHR target_present_mode = VK_PRESENT_MODE_FIFO_KHR;

			std::string name;
			uint32_t w, h;
			int resizeable;
		};

	protected:

		uint32_t _width, _height;
		GLFWwindow* _window;

		VkSurfaceKHR _surface;
		VkSwapchainKHR _swapchain;
		std::vector<std::shared_ptr<Image>> _swapchain_images;
		VkFormat _swapchain_image_format;
		VkExtent2D _swapchain_extent;
		std::vector<std::shared_ptr<ImageView>> _swapchain_views;

		std::set<uint32_t> _queues_families_indices;

		bool _framebuffer_resized = false;


		size_t _max_frames_in_flight;
		// Of size _MAX_FRAMES_IN_FLIGHT 
		std::vector<std::shared_ptr<Semaphore>> _image_available;
		std::vector<std::shared_ptr<Fence>> _in_flight_fence;
		size_t _current_frame = 0;
		// Of size swapchain (One per swap image)
		std::vector<std::shared_ptr<Fence>> _image_in_flight_fence;

		VkPresentModeKHR _target_present_mode;

		struct FrameInfo
		{
			uint32_t index;
		};

		FrameInfo _current_frame_info;

		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

		void initGLFW(std::string const& name, int resizeable);

		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> present_modes;
		};

		SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		virtual VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);

		virtual VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes, VkPresentModeKHR target);

		virtual VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		void createSwapchain(VkPresentModeKHR target);

		void createSwapchainViews();

		void createSynchObjects();

		virtual void init(CreateInfo const& ci);

		void reCreateSwapchain();

		void initSwapchain();

		void cleanupSwapchain();

		virtual void cleanup();


	public:


		VkWindow(CreateInfo const& ci);

		// The window is not copyable because of the glfw callback that need the same pointer (though it could be handled)
		VkWindow(VkWindow const&) = delete;

		VkWindow(VkWindow&&) = delete;

		VkWindow& operator=(VkWindow const&) = delete;

		VkWindow& operator=(VkWindow&&) = delete;

		virtual ~VkWindow();

		bool shouldClose()const;

		void pollEvents();

		bool framebufferResized();

		std::shared_ptr<Image> image(uint32_t index);

		std::shared_ptr<ImageView> view(uint32_t index);

		constexpr std::vector<std::shared_ptr<ImageView>> const& views() const
		{
			return _swapchain_views;
		}

		VkFormat format()const;

		size_t swapchainSize()const;

		size_t framesInFlight()const;

		VkExtent2D extent()const;

		constexpr operator GLFWwindow* ()const
		{
			return _window;
		}

		constexpr GLFWwindow* handle()const
		{
			return _window;
		}

		struct AquireResult
		{
			VkBool32 success;
			uint32_t swap_index;
			uint32_t in_flight_index;
			std::shared_ptr<Semaphore> semaphore;
			std::shared_ptr<Fence> fence;

			AquireResult();

			AquireResult(uint32_t swap_index, uint32_t in_flight_index, std::shared_ptr<Semaphore> semaphore, std::shared_ptr<Fence> fence);
		};

		AquireResult aquireNextImage();

		void present(uint32_t num_semaphores, VkSemaphore* semaphores);

	};

}