#pragma once

#include "VkApplication.hpp"

#include <set>
#include <string>
#include <vector>
#include "ImageView.hpp"
#include "Semaphore.hpp"
#include "Fence.hpp"
#include <chrono>
#include "Surface.hpp"
#include "Swapchain.hpp"

namespace vkl
{

	class VkWindow : public VkObject
	{
	public:

		struct CreateInfo
		{	
			VkApplication* app;
			std::set<uint32_t> queue_families_indices;

			DynamicValue<VkPresentModeKHR> target_present_mode = VK_PRESENT_MODE_FIFO_KHR;

			std::string name;
			uint32_t w, h;
			int resizeable;
		};

	protected:

		uint32_t _width, _height;
		GLFWwindow* _window;

		
		std::shared_ptr<Surface> _surface = nullptr;
		std::shared_ptr<Swapchain> _swapchain = nullptr;
		
		
		DynamicValue<VkExtent3D> _dynamic_extent;
		std::set<uint32_t> _queues_families_indices;

		bool _framebuffer_resized = false;

		size_t _current_frame = 0;
		// Of size swapchain (One per swap image)
		//std::vector<std::shared_ptr<Fence>> _image_in_flight_fence;

		DynamicValue<VkPresentModeKHR> _target_present_mode;

		struct FrameInfo
		{
			uint32_t index;
		};

		FrameInfo _current_frame_info;

		std::chrono::time_point<std::chrono::system_clock> _present_time_point = std::chrono::system_clock::now();
		size_t _present_frame = 0;

		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

		void initGLFW(std::string const& name, int resizeable);

		void createSwapchain();

		virtual void init(CreateInfo const& ci);

		

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

		void updateDynSize();

		void setSize(uint32_t w, uint32_t h);

		std::shared_ptr<Image> image(uint32_t index);

		std::shared_ptr<ImageView> view(uint32_t index);

		constexpr std::vector<std::shared_ptr<ImageView>> const& views() const
		{
			return _swapchain->instance()->views();
		}

		VkFormat format()const;

		size_t swapchainSize()const;

		DynamicValue<VkExtent3D> extent3D() const
		{
			return _dynamic_extent;
		}

		DynamicValue<VkExtent2D> extent2D() const
		{
			return _swapchain->extent();
		}

		constexpr operator GLFWwindow* ()const
		{
			return _window;
		}

		constexpr GLFWwindow* handle()const
		{
			return _window;
		}

		const std::shared_ptr<Swapchain>& swapchain() const
		{
			return _swapchain;
		}

		struct AquireResult
		{
			VkBool32 success;
			uint32_t swap_index;
			//uint32_t in_flight_index;
			//std::shared_ptr<Semaphore> semaphore;
			//std::shared_ptr<Fence> fence;

			AquireResult();

			AquireResult(uint32_t swap_index);
		};

		AquireResult aquireNextImage(std::shared_ptr<Semaphore> semaphore_to_signal, std::shared_ptr<Fence> fence_to_signal);

		void present(uint32_t num_semaphores, VkSemaphore* semaphores);

		bool updateResources(UpdateContext & ctx);



	};

}